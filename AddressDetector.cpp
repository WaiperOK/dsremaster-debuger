#include "pch.h"
#include "AddressDetector.h"
#include "MemoryManager.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include <windows.h>
#include <psapi.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>

struct ScanPhase {
    std::string name;
    int duration;
    std::string instruction;
};

struct AddressTracker {
    uintptr_t address;
    std::vector<std::vector<CameraCoordinates>> phaseSnapshots;
    std::vector<int> phaseChanges;
    float totalVariance;
    int totalChanges;
    std::string detectedType;
};

namespace AddressDetector {

    GameAddresses AutoDetectAll() {
        GameAddresses addresses;
        ConsoleUtils::LogInfo("Запуск многофазного сканирования для определения адресов");

        std::vector<ScanPhase> phases = {
            {"ФАЗА 1: СТАТИЧЕСКАЯ", 5, "НЕ ДВИГАЙТЕСЬ, стойте неподвижно"},
            {"ФАЗА 2: ДВИЖЕНИЕ ИГРОКА", 8, "ДВИГАЙТЕСЬ (W/A/S/D), НЕ поворачивайте камеру"},
            {"ФАЗА 3: ПОВОРОТ КАМЕРЫ", 8, "НЕ двигайтесь, только ПОВОРАЧИВАЙТЕ КАМЕРУ мышью"},
            {"ФАЗА 4: СМЕШАННОЕ", 6, "Свободно ДВИГАЙТЕСЬ И ПОВОРАЧИВАЙТЕ КАМЕРУ"}
        };

        HMODULE hModule = GetModuleHandle(nullptr);
        if (!hModule) {
            ConsoleUtils::LogError("Не удалось получить дескриптор модуля");
            return addresses;
        }

        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
            ConsoleUtils::LogError("Не удалось получить информацию о модуле");
            return addresses;
        }

        uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
        uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;

        std::vector<AddressTracker> candidates;

        ConsoleUtils::LogInfo("Поиск потенциальных адресов...");
        for (uintptr_t addr = startAddress; addr < endAddress - 12; addr += 16) {
            if (!MemoryUtils::IsPageAccessible(addr, sizeof(float) * 3))
                continue;

            try {
                CameraCoordinates coords = ReadCameraCoordinates(addr);

                constexpr float MAX_COORD = 10000.0f;
                if (std::abs(coords.x) < MAX_COORD && std::abs(coords.y) < MAX_COORD &&
                    std::abs(coords.z) < MAX_COORD &&
                    std::abs(coords.x) > 0.001f && std::abs(coords.y) > 0.001f && std::abs(coords.z) > 0.001f) {

                    AddressTracker tracker;
                    tracker.address = addr;
                    tracker.phaseSnapshots.resize(phases.size());
                    tracker.phaseChanges.resize(phases.size(), 0);
                    tracker.totalVariance = 0.0f;
                    tracker.totalChanges = 0;
                    tracker.detectedType = "НЕИЗВЕСТНО";
                    candidates.push_back(tracker);
                }
            } catch (...) {
                continue;
            }
        }

        ConsoleUtils::LogInfo("Найдено " + std::to_string(candidates.size()) + " кандидатов");

        for (size_t phaseIdx = 0; phaseIdx < phases.size(); phaseIdx++) {
            const ScanPhase& phase = phases[phaseIdx];

            ConsoleUtils::LogInfo("[" + std::to_string(phaseIdx + 1) + "/" + std::to_string(phases.size()) + "] " + phase.name + " (" + std::to_string(phase.duration) + " сек)");
            ConsoleUtils::LogInfo("ИНСТРУКЦИЯ: " + phase.instruction);
            ConsoleUtils::LogInfo("Подготовьтесь... начинаем через 3 сек");
            Sleep(3000);

            const int SNAPSHOTS_PER_PHASE = phase.duration * 2;
            const int SLEEP_MS = 500;

            for (auto& candidate : candidates) {
                if (!MemoryUtils::IsPageAccessible(candidate.address, sizeof(float) * 3))
                    continue;
                try {
                    CameraCoordinates coords = ReadCameraCoordinates(candidate.address);
                    candidate.phaseSnapshots[phaseIdx].push_back(coords);
                } catch (...) {
                    continue;
                }
            }

            for (int snapshot = 0; snapshot < SNAPSHOTS_PER_PHASE; snapshot++) {
                Sleep(SLEEP_MS);

                // Прогресс-бар
                int progress = (snapshot + 1) * 100 / SNAPSHOTS_PER_PHASE;
                std::cout << "\r[";
                for (int i = 0; i < 30; i++) {
                    std::cout << (i < progress / 3 ? "=" : " ");
                }
                std::cout << "] " << progress << "%";
                std::cout.flush();

                for (auto& candidate : candidates) {
                    if (!MemoryUtils::IsPageAccessible(candidate.address, sizeof(float) * 3))
                        continue;

                    try {
                        CameraCoordinates coords = ReadCameraCoordinates(candidate.address);

                        if (!candidate.phaseSnapshots[phaseIdx].empty()) {
                            CameraCoordinates prev = candidate.phaseSnapshots[phaseIdx].back();
                            float deltaX = std::abs(coords.x - prev.x);
                            float deltaY = std::abs(coords.y - prev.y);
                            float deltaZ = std::abs(coords.z - prev.z);

                            if (deltaX > 0.01f || deltaY > 0.01f || deltaZ > 0.01f) {
                                candidate.phaseChanges[phaseIdx]++;
                            }
                        }

                        candidate.phaseSnapshots[phaseIdx].push_back(coords);
                    } catch (...) {
                        continue;
                    }
                }
            }

            std::cout << "\n";
            ConsoleUtils::LogSuccess("ФАЗА " + std::to_string(phaseIdx + 1) + " ЗАВЕРШЕНА");
        }

        ConsoleUtils::LogInfo("Анализ результатов сканирования...");

        for (auto& candidate : candidates) {
            candidate.totalChanges = 0;
            for (int changes : candidate.phaseChanges) {
                candidate.totalChanges += changes;
            }

            int staticPhase = candidate.phaseChanges[0];
            int playerPhase = candidate.phaseChanges[1];
            int cameraPhase = candidate.phaseChanges[2];
            int mixedPhase = candidate.phaseChanges[3];

            if (staticPhase <= 1 && playerPhase >= 3 && cameraPhase <= 2) {
                candidate.detectedType = "ИГРОК";
                addresses.playerAddresses.push_back(candidate.address);
            } else if (staticPhase <= 1 && cameraPhase >= 4 && (playerPhase <= 2 || mixedPhase >= 6)) {
                candidate.detectedType = "КАМЕРА";
                addresses.cameraAddresses.push_back(candidate.address);
            } else if (playerPhase >= 3 && cameraPhase >= 3 && mixedPhase >= 5) {
                candidate.detectedType = "УНИВЕРСАЛЬНЫЙ";
                addresses.cameraAddresses.push_back(candidate.address);
            } else if (candidate.totalChanges >= 5) {
                candidate.detectedType = "ДИНАМИЧЕСКИЙ";
            } else {
                candidate.detectedType = "СТАТИЧЕСКИЙ";
            }
        }

        std::sort(candidates.begin(), candidates.end(),
                  [](const AddressTracker& a, const AddressTracker& b) {
                      return a.totalChanges > b.totalChanges;
                  });

        ConsoleUtils::LogSuccess("РЕЗУЛЬТАТЫ АНАЛИЗА:");
        ConsoleUtils::LogInfo("Адресов ИГРОКА: " + std::to_string(addresses.playerAddresses.size()));
        ConsoleUtils::LogInfo("Адресов КАМЕРЫ: " + std::to_string(addresses.cameraAddresses.size()));

        int shown = 0;
        for (const auto& candidate : candidates) {
            if (candidate.totalChanges > 0 && shown < 10) {
                std::cout << "  [" << candidate.detectedType << "] 0x" << std::hex << candidate.address << std::dec
                          << " (измен: " << candidate.totalChanges << ")" << std::endl;
                shown++;
            }
        }

        return addresses;
    }

    std::vector<uintptr_t> AutoDetectCameraAddresses() {
        GameAddresses addresses = AutoDetectAll();
        std::vector<uintptr_t> combined;

        combined.insert(combined.end(), addresses.playerAddresses.begin(), addresses.playerAddresses.end());
        combined.insert(combined.end(), addresses.cameraAddresses.begin(), addresses.cameraAddresses.end());

        return combined;
    }
}
