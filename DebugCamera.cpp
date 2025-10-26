
#include "pch.h"
#include "DebugCamera.h"
#include "MemoryManager.h"
#include "StringUtils.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include "ConsoleUI.h"

#include <windows.h>
#include <psapi.h>        // Для GetModuleInformation
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <thread>
#include <chrono>
#include <iomanip>
#include <limits>         // Для std::numeric_limits
#include <algorithm>      // Для std::min
#include <conio.h>        // Для _kbhit() и _getch()
#include <cstdlib>        // Для rand()
#include <ctime>          // Для time()
#include <map>            // Для std::map

#undef min
#undef max


// Функция для надежного чтения из консоли
std::string ReadConsoleInput() {
    char buffer[256];
    DWORD bytesRead;
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    
    if (ReadConsoleA(hStdin, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        buffer[bytesRead] = '\0';
        // Убираем символы переноса строки
        for (DWORD i = 0; i < bytesRead; i++) {
            if (buffer[i] == '\r' || buffer[i] == '\n') {
                buffer[i] = '\0';
                break;
            }
        }
        return std::string(buffer);
    }
    return "";
}

// Функция для чтения трех float чисел
bool ReadThreeFloats(float& x, float& y, float& z) {
    std::string input = ReadConsoleInput();
    std::istringstream iss(input);
    if (iss >> x >> y >> z) {
        return true;
    }
    return false;
}


// Сканирует память указанного модуля в поиске шаблона с заданной маской,
// возвращая все найденные адреса.
std::vector<uintptr_t> FindAllPatterns(HMODULE hModule, const char* pattern, const char* mask) {
    std::vector<uintptr_t> addresses;
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        std::cerr << "Не удалось получить информацию о модуле." << std::endl;
        return addresses;
    }
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
    size_t patternLength = strlen(mask);

    for (uintptr_t current = startAddress; current < endAddress - patternLength; current++) {
        bool found = true;
        for (size_t i = 0; i < patternLength; i++) {
            if (mask[i] == 'x' && *reinterpret_cast<char*>(current + i) != pattern[i]) {
                found = false;
                break;
            }
        }
        if (found)
            addresses.push_back(current);
    }
    return addresses;
}

// Структура для отслеживания изменений значений
struct MemorySnapshot {
    uintptr_t address;
    float values[3];
    int changeCount;
};

// Структура для хранения найденных адресов персонажа и камеры
struct GameAddresses {
    std::vector<uintptr_t> playerAddresses;
    std::vector<uintptr_t> cameraAddresses;
};

// Многократное сканирование для точного определения типа адреса
struct ScanPhase {
    std::string name;
    int duration;
    std::string instruction;
};

GameAddresses MultipassAutoDetect() {
    GameAddresses addresses;
    std::cout << "[*] Запуск МНОГОФАЗНОГО СКАНИРОВАНИЯ для точного определения адресов..." << std::endl;
    
    // Определяем фазы сканирования
    std::vector<ScanPhase> phases = {
        {"ФАЗА 1: СТАТИЧЕСКАЯ", 5, "НЕ ДВИГАЙТЕСЬ, стойте неподвижно"},
        {"ФАЗА 2: ДВИЖЕНИЕ ИГРОКА", 8, "ДВИГАЙТЕСЬ (W/A/S/D), НЕ поворачивайте камеру"},
        {"ФАЗА 3: ПОВОРОТ КАМЕРЫ", 8, "НЕ двигайтесь, только ПОВОРАЧИВАЙТЕ КАМЕРУ мышью"},
        {"ФАЗА 4: СМЕШАННОЕ", 6, "Свободно ДВИГАЙТЕСЬ И ПОВОРАЧИВАЙТЕ КАМЕРУ"}
    };
    
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule) {
        std::cerr << "Не удалось получить дескриптор модуля." << std::endl;
        return addresses;
    }
    
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        std::cerr << "Не удалось получить информацию о модуле." << std::endl;
        return addresses;
    }
    
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
    
    // Структура для отслеживания адресов через все фазы
    struct AddressTracker {
        uintptr_t address;
        std::vector<std::vector<CameraCoordinates>> phaseSnapshots;
        std::vector<int> phaseChanges;
        float totalVariance;
        int totalChanges;
        std::string detectedType;
    };
    
    std::vector<AddressTracker> candidates;
    
    // Первоначальный поиск кандидатов
    std::cout << "[*] Поиск потенциальных адресов..." << std::endl;
    for (uintptr_t addr = startAddress; addr < endAddress - 12; addr += 16) {
        try {
            float* ptr = reinterpret_cast<float*>(addr);
            if (abs(ptr[0]) < 10000.0f && abs(ptr[1]) < 10000.0f && abs(ptr[2]) < 10000.0f &&
                ptr[0] != 0.0f && ptr[1] != 0.0f && ptr[2] != 0.0f) {
                
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
    
    std::cout << "[*] Найдено " << candidates.size() << " кандидатов для многофазного анализа" << std::endl;
    
    // Выполняем каждую фазу сканирования
    for (size_t phaseIdx = 0; phaseIdx < phases.size(); phaseIdx++) {
        const ScanPhase& phase = phases[phaseIdx];
        
        std::cout << "\n" << phase.name << " (" << phase.duration << " сек)" << std::endl;
        std::cout << "ИНСТРУКЦИЯ: " << phase.instruction << std::endl;
        std::cout << "Подготовьтесь... Начинаем через 3 секунды..." << std::endl;
        Sleep(3000);
        
        const int SNAPSHOTS_PER_PHASE = phase.duration * 2; // 2 снимка в секунду
        const int SLEEP_MS = 500;
        
        // Сохраняем начальное состояние для этой фазы
        for (auto& candidate : candidates) {
            try {
                CameraCoordinates coords = ReadCameraCoordinates(candidate.address);
                candidate.phaseSnapshots[phaseIdx].push_back(coords);
            } catch (...) {
                continue;
            }
        }
        
        // Мониторинг изменений в этой фазе
        for (int snapshot = 0; snapshot < SNAPSHOTS_PER_PHASE; snapshot++) {
            Sleep(SLEEP_MS);
            std::cout << "•";
            std::cout.flush();
            
            for (auto& candidate : candidates) {
                try {
                    CameraCoordinates coords = ReadCameraCoordinates(candidate.address);
                    
                    if (!candidate.phaseSnapshots[phaseIdx].empty()) {
                        CameraCoordinates prev = candidate.phaseSnapshots[phaseIdx].back();
                        float deltaX = abs(coords.x - prev.x);
                        float deltaY = abs(coords.y - prev.y);
                        float deltaZ = abs(coords.z - prev.z);
                        
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
        
        std::cout << " ЗАВЕРШЕНО" << std::endl;
    }
    
    std::cout << "\n[*] Анализ результатов многофазного сканирования..." << std::endl;
    
    // Анализ паттернов поведения по фазам
    for (auto& candidate : candidates) {
        candidate.totalChanges = 0;
        for (int changes : candidate.phaseChanges) {
            candidate.totalChanges += changes;
        }
        
        // Определяем тип на основе паттернов активности по фазам
        int staticPhase = candidate.phaseChanges[0];    // Фаза 1: статическая
        int playerPhase = candidate.phaseChanges[1];    // Фаза 2: движение игрока
        int cameraPhase = candidate.phaseChanges[2];    // Фаза 3: поворот камеры
        int mixedPhase = candidate.phaseChanges[3];     // Фаза 4: смешанное
        
        // Улучшенная классификация на основе многофазного анализа
        if (staticPhase <= 1 && playerPhase >= 3 && cameraPhase <= 2) {
            // Изменяется при движении игрока, но не при вращении камеры
            candidate.detectedType = "ИГРОК";
            addresses.playerAddresses.push_back(candidate.address);
        } else if (staticPhase <= 1 && cameraPhase >= 4 && (playerPhase <= 2 || mixedPhase >= 6)) {
            // Изменяется при вращении камеры
            candidate.detectedType = "КАМЕРА";
            addresses.cameraAddresses.push_back(candidate.address);
        } else if (playerPhase >= 3 && cameraPhase >= 3 && mixedPhase >= 5) {
            // Изменяется при любом движении - может быть универсальным адресом
            candidate.detectedType = "УНИВЕРСАЛЬНЫЙ";
            addresses.cameraAddresses.push_back(candidate.address); // Добавляем как камеру
        } else if (candidate.totalChanges >= 5) {
            candidate.detectedType = "ДИНАМИЧЕСКИЙ";
        } else {
            candidate.detectedType = "СТАТИЧЕСКИЙ";
        }
    }
    
    // Сортируем по общей активности
    std::sort(candidates.begin(), candidates.end(), 
              [](const AddressTracker& a, const AddressTracker& b) {
                  return a.totalChanges > b.totalChanges;
              });
    
    // Отображаем результаты анализа
    std::cout << "\n[+] РЕЗУЛЬТАТЫ МНОГОФАЗНОГО АНАЛИЗА:" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    int shown = 0;
    for (const auto& candidate : candidates) {
        if (candidate.totalChanges > 0 && shown < 20) { // Показываем только активные адреса
            std::cout << candidate.detectedType << " 0x" << std::hex << candidate.address << std::dec;
            std::cout << " [Статич:" << candidate.phaseChanges[0];
            std::cout << ", Игрок:" << candidate.phaseChanges[1];
            std::cout << ", Камера:" << candidate.phaseChanges[2];
            std::cout << ", Смеш:" << candidate.phaseChanges[3];
            std::cout << ", Всего:" << candidate.totalChanges << "]" << std::endl;
            shown++;
        }
    }
    
    std::cout << "\n[+] Классифицировано:" << std::endl;
    std::cout << "- Адресов ИГРОКА: " << addresses.playerAddresses.size() << std::endl;
    std::cout << "- Адресов КАМЕРЫ: " << addresses.cameraAddresses.size() << std::endl;
    
    return addresses;
}

// Расширенный анализ адресов для определения их точного назначения
struct ExtendedAddressInfo {
    uintptr_t address;
    std::string primaryType;      // ИГРОК, КАМЕРА, НЕИЗВЕСТНО
    std::string secondaryType;    // ПОЗИЦИЯ, УГЛЫ, НАПРАВЛЕНИЕ, СКОРОСТЬ
    std::string detailedType;     // Подробное описание
    float avgX, avgY, avgZ;       // Средние значения
    float varX, varY, varZ;       // Дисперсия
    int changeFrequency;
    float stability;
    bool isAngular;               // Содержит углы
    bool isPosition;              // Содержит позицию
    bool isGroundLevel;           // На уровне земли
    bool isHighFrequency;         // Высокочастотные изменения
};

std::vector<ExtendedAddressInfo> AnalyzeAddressTypes(const std::vector<uintptr_t>& addresses) {
    std::vector<ExtendedAddressInfo> results;
    std::cout << "[*] Запуск РАСШИРЕННОГО АНАЛИЗА типов адресов..." << std::endl;
    std::cout << "[*] Это займет 12 секунд. Активно двигайтесь и поворачивайте камеру!" << std::endl;
    
    const int ANALYSIS_DURATION = 12;
    const int SNAPSHOTS = 24;
    const int SLEEP_MS = (ANALYSIS_DURATION * 1000) / SNAPSHOTS;
    
    // Инициализация результатов
    for (uintptr_t addr : addresses) {
        ExtendedAddressInfo info;
        info.address = addr;
        info.primaryType = "НЕИЗВЕСТНО";
        info.secondaryType = "ДАННЫЕ";
        info.detailedType = "Неклассифицировано";
        info.avgX = info.avgY = info.avgZ = 0.0f;
        info.varX = info.varY = info.varZ = 0.0f;
        info.changeFrequency = 0;
        info.stability = 0.0f;
        info.isAngular = false;
        info.isPosition = false;
        info.isGroundLevel = false;
        info.isHighFrequency = false;
        results.push_back(info);
    }
    
    // Сбор данных
    std::vector<std::vector<CameraCoordinates>> snapshots(addresses.size());
    
    for (int snapshot = 0; snapshot < SNAPSHOTS; snapshot++) {
        Sleep(SLEEP_MS);
        std::cout << "•";
        std::cout.flush();
        
        for (size_t i = 0; i < addresses.size(); i++) {
            try {
                CameraCoordinates coords = ReadCameraCoordinates(addresses[i]);
                
                // Подсчет изменений
                if (!snapshots[i].empty()) {
                    CameraCoordinates prev = snapshots[i].back();
                    if (abs(coords.x - prev.x) > 0.01f || 
                        abs(coords.y - prev.y) > 0.01f || 
                        abs(coords.z - prev.z) > 0.01f) {
                        results[i].changeFrequency++;
                    }
                }
                
                snapshots[i].push_back(coords);
            } catch (...) {
                continue;
            }
        }
    }
    
    std::cout << std::endl << "[*] Анализ паттернов данных..." << std::endl;
    
    // Детальный анализ каждого адреса
    for (size_t i = 0; i < results.size(); i++) {
        ExtendedAddressInfo& info = results[i];
        
        if (snapshots[i].size() < 3) continue; // Недостаточно данных
        
        // Вычисляем статистики
        for (const auto& coords : snapshots[i]) {
            info.avgX += coords.x;
            info.avgY += coords.y;
            info.avgZ += coords.z;
        }
        info.avgX /= snapshots[i].size();
        info.avgY /= snapshots[i].size();
        info.avgZ /= snapshots[i].size();
        
        // Вычисляем дисперсию
        for (const auto& coords : snapshots[i]) {
            info.varX += (coords.x - info.avgX) * (coords.x - info.avgX);
            info.varY += (coords.y - info.avgY) * (coords.y - info.avgY);
            info.varZ += (coords.z - info.avgZ) * (coords.z - info.avgZ);
        }
        info.varX /= snapshots[i].size();
        info.varY /= snapshots[i].size();
        info.varZ /= snapshots[i].size();
        
        info.stability = 1.0f / (1.0f + info.varX + info.varY + info.varZ);
        
        // Определяем характеристики данных
        info.isAngular = (abs(info.avgX) <= 6.28f && abs(info.avgY) <= 6.28f && abs(info.avgZ) <= 6.28f) ||
                        (abs(info.avgX) <= 360.0f && abs(info.avgY) <= 360.0f && abs(info.avgZ) <= 360.0f);
        
        info.isPosition = (abs(info.avgX) < 5000.0f && abs(info.avgY) < 1000.0f && abs(info.avgZ) < 5000.0f);
        info.isGroundLevel = (info.avgY > -100.0f && info.avgY < 200.0f);
        info.isHighFrequency = (info.changeFrequency >= 10);
        
        // ПРОДВИНУТАЯ КЛАССИФИКАЦИЯ
        if (info.isAngular && info.changeFrequency >= 5) {
            info.primaryType = "КАМЕРА";
            info.secondaryType = "УГЛЫ";
            if (info.varX > 0.5f || info.varY > 0.5f) {
                info.detailedType = "Углы поворота камеры (активные)";
            } else {
                info.detailedType = "Углы поворота камеры (статические)";
            }
        }
        else if (info.isPosition && info.isGroundLevel && info.changeFrequency >= 3 && info.changeFrequency < 10) {
            info.primaryType = "ИГРОК";
            info.secondaryType = "ПОЗИЦИЯ";
            if (info.varX > 10.0f || info.varZ > 10.0f) {
                info.detailedType = "Позиция игрока (активное движение)";
            } else {
                info.detailedType = "Позиция игрока (медленное движение)";
            }
        }
        else if (info.isPosition && !info.isGroundLevel && info.isHighFrequency) {
            info.primaryType = "КАМЕРА";
            info.secondaryType = "ПОЗИЦИЯ";
            if (abs(info.avgY) > 200.0f) {
                info.detailedType = "Позиция свободной камеры";
            } else {
                info.detailedType = "Позиция камеры следования";
            }
        }
        else if (info.changeFrequency >= 15) {
            info.primaryType = "КАМЕРА";
            info.secondaryType = "ВЫСОКОЧАСТОТНЫЕ";
            info.detailedType = "Высокочастотные данные камеры";
        }
        else if (info.isPosition && info.changeFrequency >= 8) {
            info.primaryType = "УНИВЕРСАЛЬНЫЙ";
            info.secondaryType = "КООРДИНАТЫ";
            info.detailedType = "Универсальные координаты (игрок+камера)";
        }
        else if (info.changeFrequency >= 3) {
            info.primaryType = "ДИНАМИЧЕСКИЙ";
            info.secondaryType = "НЕИЗВЕСТНО";
            info.detailedType = "Динамические данные (неопределено)";
        }
        else {
            info.primaryType = "СТАТИЧЕСКИЙ";
            info.secondaryType = "ДАННЫЕ";
            info.detailedType = "Статические или медленно изменяющиеся данные";
        }
    }
    
    // Сортируем по релевантности (частота изменений + стабильность)
    std::sort(results.begin(), results.end(), 
              [](const ExtendedAddressInfo& a, const ExtendedAddressInfo& b) {
                  float scoreA = a.changeFrequency + a.stability * 5;
                  float scoreB = b.changeFrequency + b.stability * 5;
                  return scoreA > scoreB;
              });
    
    return results;
}

// Умный автоматический тест всех найденных адресов с детальной классификацией
struct AddressTestResult {
    uintptr_t address;
    bool isResponsive;
    bool isPlayer;
    bool isCamera;
    float stability;
    int changeFrequency;
    std::string type;
};

std::vector<AddressTestResult> SmartTestAllAddresses(const std::vector<uintptr_t>& addresses) {
    std::vector<AddressTestResult> results;
    std::cout << "[*] Запуск УМНОГО ТЕСТА всех " << addresses.size() << " адресов..." << std::endl;
    std::cout << "[*] Это займет 15 секунд. Пожалуйста, АКТИВНО ДВИГАЙТЕСЬ в игре!" << std::endl;
    
    const int TEST_DURATION = 15; // секунд
    const int SNAPSHOTS = 30; // количество снимков
    const int SLEEP_MS = (TEST_DURATION * 1000) / SNAPSHOTS;
    
    // Инициализация результатов
    for (uintptr_t addr : addresses) {
        AddressTestResult result;
        result.address = addr;
        result.isResponsive = false;
        result.isPlayer = false;
        result.isCamera = false;
        result.stability = 0.0f;
        result.changeFrequency = 0;
        result.type = "НЕИЗВЕСТНО";
        results.push_back(result);
    }
    
    // Сохраняем начальные значения
    std::vector<std::vector<CameraCoordinates>> snapshots(addresses.size());
    for (size_t i = 0; i < addresses.size(); i++) {
        try {
            CameraCoordinates coords = ReadCameraCoordinates(addresses[i]);
            snapshots[i].push_back(coords);
        } catch (...) {
            continue;
        }
    }
    
    // Мониторинг изменений
    for (int snapshot = 0; snapshot < SNAPSHOTS; snapshot++) {
        Sleep(SLEEP_MS);
        std::cout << "Снимок [" << (snapshot + 1) << "/" << SNAPSHOTS << "] ";
        std::cout.flush();
        
        for (size_t i = 0; i < addresses.size(); i++) {
            try {
                CameraCoordinates coords = ReadCameraCoordinates(addresses[i]);
                
                // Проверяем изменения с предыдущим снимком
                if (!snapshots[i].empty()) {
                    CameraCoordinates prev = snapshots[i].back();
                    float deltaX = abs(coords.x - prev.x);
                    float deltaY = abs(coords.y - prev.y);
                    float deltaZ = abs(coords.z - prev.z);
                    
                    if (deltaX > 0.01f || deltaY > 0.01f || deltaZ > 0.01f) {
                        results[i].changeFrequency++;
                    }
                }
                
                snapshots[i].push_back(coords);
            } catch (...) {
                continue;
            }
        }
        
        if ((snapshot + 1) % 10 == 0) {
            std::cout << std::endl;
        }
    }
    
    std::cout << std::endl << "[*] Анализ результатов..." << std::endl;
    
    // Анализ и классификация адресов
    for (size_t i = 0; i < results.size(); i++) {
        AddressTestResult& result = results[i];
        
        if (result.changeFrequency >= 3) {
            result.isResponsive = true;
            
            // Анализ паттернов движения для классификации
            if (!snapshots[i].empty()) {
                float avgX = 0, avgY = 0, avgZ = 0;
                float varianceX = 0, varianceY = 0, varianceZ = 0;
                
                // Вычисляем средние значения
                for (const auto& coords : snapshots[i]) {
                    avgX += coords.x;
                    avgY += coords.y;
                    avgZ += coords.z;
                }
                avgX /= snapshots[i].size();
                avgY /= snapshots[i].size();
                avgZ /= snapshots[i].size();
                
                // Вычисляем дисперсию
                for (const auto& coords : snapshots[i]) {
                    varianceX += (coords.x - avgX) * (coords.x - avgX);
                    varianceY += (coords.y - avgY) * (coords.y - avgY);
                    varianceZ += (coords.z - avgZ) * (coords.z - avgZ);
                }
                varianceX /= snapshots[i].size();
                varianceY /= snapshots[i].size();
                varianceZ /= snapshots[i].size();
                
                result.stability = 1.0f / (1.0f + varianceX + varianceY + varianceZ);
                
                // Улучшенная эвристика классификации
                if (abs(avgY) < 100.0f && abs(avgX) < 1000.0f && abs(avgZ) < 1000.0f &&
                    varianceY < 50.0f && result.changeFrequency >= 5) {
                    result.isPlayer = true;
                    result.type = "ИГРОК";
                } else if (result.changeFrequency >= 8 || varianceX > 100.0f || varianceZ > 100.0f) {
                    result.isCamera = true;
                    result.type = "КАМЕРА";
                } else if (result.changeFrequency >= 3) {
                    result.type = "ДИНАМИЧЕСКИЙ";
                }
            }
        } else {
            result.type = "СТАТИЧЕСКИЙ";
        }
    }
    
    // Сортируем результаты по частоте изменений
    std::sort(results.begin(), results.end(), 
              [](const AddressTestResult& a, const AddressTestResult& b) {
                  return a.changeFrequency > b.changeFrequency;
              });
    
    return results;
}

// Реальный автодетект камеры и персонажа по изменяющимся координатам
GameAddresses RealAutoDetectAll() {
    GameAddresses addresses;
    std::cout << "[*] Запуск РЕАЛЬНОГО автоопределения для ИГРОКА и КАМЕРЫ..." << std::endl;
    std::cout << "[*] Это займет 10 секунд. Пожалуйста, ДВИГАЙТЕСЬ и ПОВОРАЧИВАЙТЕ КАМЕРУ в игре!" << std::endl;
    std::cout.flush();
    
    HMODULE hModule = GetModuleHandle(nullptr);
    if (!hModule) {
        std::cerr << "Не удалось получить дескриптор модуля." << std::endl;
        return addresses;
    }
    
    MODULEINFO moduleInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
        std::cerr << "Не удалось получить информацию о модуле." << std::endl;
        return addresses;
    }
    
    uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
    
    std::cout << "[*] Сканирование диапазона памяти: 0x" << std::hex << startAddress << " - 0x" << endAddress << std::endl;
    std::cout << "[*] Поиск изменяющихся float значений..." << std::endl;
    std::cout.flush();
    
    std::vector<MemorySnapshot> candidates;
    const int SCAN_DURATION = 10; // секунд
    const int SNAPSHOTS = 20; // количество снимков
    const int SLEEP_MS = (SCAN_DURATION * 1000) / SNAPSHOTS;
    
    // Первоначальный поиск float значений
    for (uintptr_t addr = startAddress; addr < endAddress - 12; addr += 16) { // шаг 16 для оптимизации
        try {
            float* ptr = reinterpret_cast<float*>(addr);
            // Проверяем, что это похоже на координаты (разумные значения)
            if (abs(ptr[0]) < 10000.0f && abs(ptr[1]) < 10000.0f && abs(ptr[2]) < 10000.0f &&
                ptr[0] != 0.0f && ptr[1] != 0.0f && ptr[2] != 0.0f) {
                
                MemorySnapshot snapshot;
                snapshot.address = addr;
                snapshot.values[0] = ptr[0];
                snapshot.values[1] = ptr[1];
                snapshot.values[2] = ptr[2];
                snapshot.changeCount = 0;
                candidates.push_back(snapshot);
            }
        }
        catch (...) {
            // Игнорируем недоступные адреса
            continue;
        }
    }
    
    std::cout << "[*] Найдено " << candidates.size() << " потенциальных float троек" << std::endl;
    std::cout << "[*] Отслеживание изменений..." << std::endl;
    std::cout.flush();
    
    // Мониторинг изменений
    for (int snapshot = 0; snapshot < SNAPSHOTS; snapshot++) {
        Sleep(SLEEP_MS);
        std::cout << "[" << (snapshot + 1) << "/" << SNAPSHOTS << "] ";
        std::cout.flush();
        
        for (auto& candidate : candidates) {
            try {
                float* ptr = reinterpret_cast<float*>(candidate.address);
                
                // Проверяем изменились ли значения
                if (abs(ptr[0] - candidate.values[0]) > 0.01f ||
                    abs(ptr[1] - candidate.values[1]) > 0.01f ||
                    abs(ptr[2] - candidate.values[2]) > 0.01f) {
                    
                    candidate.changeCount++;
                    candidate.values[0] = ptr[0];
                    candidate.values[1] = ptr[1];
                    candidate.values[2] = ptr[2];
                }
            }
            catch (...) {
                // Адрес стал недоступен
                continue;
            }
        }
        
        if ((snapshot + 1) % 5 == 0) {
            std::cout << std::endl;
            std::cout.flush();
        }
    }
    
    std::cout << std::endl << "[*] Анализ завершен! Поиск часто изменяющихся адресов..." << std::endl;
    
    // Анализ результатов - ищем адреса с частыми изменениями
    std::vector<std::pair<uintptr_t, int>> bestCandidates;
    for (const auto& candidate : candidates) {
        if (candidate.changeCount >= 3) { // Изменялись хотя бы 3 раза
            bestCandidates.push_back({candidate.address, candidate.changeCount});
        }
    }
    
    // Сортируем по количеству изменений
    std::sort(bestCandidates.begin(), bestCandidates.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::cout << "[*] Найдено " << bestCandidates.size() << " динамических адресов:" << std::endl;
    std::cout << "[*] Анализ адресов для разделения ИГРОКА от КАМЕРЫ..." << std::endl;
    
    for (size_t i = 0; i < std::min(bestCandidates.size(), size_t(15)); i++) {
        uintptr_t addr = bestCandidates[i].first;
        int changes = bestCandidates[i].second;
        
        try {
            float* ptr = reinterpret_cast<float*>(addr);
            float x = ptr[0], y = ptr[1], z = ptr[2];
            std::string type;

            // Эвристика для определения типа адреса
            bool isPlayer = false;
            bool isCamera = false;
            
            // УЛУЧШЕННАЯ ЭВРИСТИКА для точного определения типа адреса
            
            // Анализ диапазонов значений для более точной классификации
            bool hasReasonablePlayerCoords = (abs(y) < 200.0f && abs(x) < 2000.0f && abs(z) < 2000.0f);
            bool hasGroundLevelY = (y > -50.0f && y < 100.0f); // Игрок обычно на уровне земли
            bool hasHighVariability = (changes >= 10); // Очень активные изменения
            bool hasModerateActivity = (changes >= 5 && changes < 10); // Умеренная активность
            bool hasLowActivity = (changes >= 3 && changes < 5); // Низкая активность
            
            // Определяем углы камеры (обычно в диапазоне -PI до +PI или 0-360 градусов)
            bool likelyAngles = ((abs(x) <= 6.28f || abs(x) <= 360.0f) && 
                               (abs(y) <= 6.28f || abs(y) <= 360.0f) && 
                               (abs(z) <= 6.28f || abs(z) <= 360.0f));
            
            // Проверка на возможные координаты позиции игрока
            if (hasReasonablePlayerCoords && hasGroundLevelY && hasModerateActivity && !likelyAngles) {
                isPlayer = true;
                std::string subtype = "ПОЗИЦИЯ";
                if (changes >= 8) subtype = "ПОЗИЦИЯ+ДВИЖЕНИЕ";
                type = "[ИГРОК:" + subtype + "]";
            }
            // Проверка на координаты камеры (обычно более активные и могут быть выше)
            else if (hasHighVariability && !hasGroundLevelY && !likelyAngles) {
                isCamera = true;
                std::string subtype = "3D-ПОЗИЦИЯ";
                if (abs(y) > 100.0f) subtype = "СВОБОДНАЯ-КАМЕРА";
                type = "[КАМЕРА:" + subtype + "]";
            }
            // Проверка на углы поворота камеры
            else if (likelyAngles && changes >= 5) {
                isCamera = true;
                type = "[КАМЕРА:УГЛЫ]";
            }
            // Проверка на смешанные координаты (может быть универсальным адресом)
            else if (hasReasonablePlayerCoords && hasHighVariability) {
                isCamera = true; // Добавляем как камеру для универсальности
                type = "[УНИВЕРСАЛЬНЫЙ]";
            }
            // Статические или слабо активные адреса
            else if (hasLowActivity) {
                std::string subtype = "СТАТИЧЕСКИЙ";
                if (hasReasonablePlayerCoords) subtype = "МЕДЛЕННЫЙ-ИГРОК";
                type = "[" + subtype + "]";
            }
            // Активные адреса с неясным назначением
            else if (changes >= 5) {
                type = "[ДИНАМИЧЕСКИЙ]";
                isCamera = true; // По умолчанию добавляем как камеру
            }
            else {
                type = "[НЕИЗВЕСТНО]";
            }
            
            std::cout << "   " << type << " 0x" << std::hex << addr << " (изменилось " << std::dec << changes << " раз)" << std::endl;
            std::cout << "     Значения: " << std::fixed << std::setprecision(2) << x << ", " << y << ", " << z << std::endl;
            
            if (isPlayer) {
                addresses.playerAddresses.push_back(addr);
            } else {
                addresses.cameraAddresses.push_back(addr);
            }
        }
        catch (...) {
            std::cout << "   [ОШИБКА] 0x" << std::hex << addr << " - Невозможно прочитать значения" << std::endl;
        }
    }
    
    std::cout << "[+] Найдено " << addresses.playerAddresses.size() << " потенциальных адресов ИГРОКА" << std::endl;
    std::cout << "[+] Найдено " << addresses.cameraAddresses.size() << " потенциальных адресов КАМЕРЫ" << std::endl;
    
    return addresses;
}

// Автоматический поиск камеры (для обратной совместимости)
std::vector<uintptr_t> AutoDetectCameraAddresses() {
    GameAddresses addresses = RealAutoDetectAll();
    std::vector<uintptr_t> combined;
    
    // Объединяем адреса персонажа и камеры
    combined.insert(combined.end(), addresses.playerAddresses.begin(), addresses.playerAddresses.end());
    combined.insert(combined.end(), addresses.cameraAddresses.begin(), addresses.cameraAddresses.end());
    
    return combined;
}

// Ищет адреса камеры с использованием заданного шаблона и маски.
// Возвращает вектор найденных адресов.
std::vector<uintptr_t> FindCameraAddresses(const std::string& currentPattern, const std::string& currentMask) {
    HMODULE hModule = GetModuleHandle(nullptr);
    std::vector<uintptr_t> addresses;
    if (!hModule) {
        std::cerr << "Не удалось получить дескриптор модуля." << std::endl;
        return addresses;
    }
    addresses = FindAllPatterns(hModule, currentPattern.c_str(), currentMask.c_str());
    if (addresses.empty())
        std::cerr << "Не удалось найти адреса камеры по заданному шаблону." << std::endl;
    else {
        // Фильтруем найденные адреса, оставляя только те, где значение magic соответствует 0x12345678.
        std::vector<uintptr_t> validAddresses;
        for (auto addr : addresses) {
            if (IsValidCameraAddress(addr))
                validAddresses.push_back(addr);
        }
        if (validAddresses.empty())
            std::cerr << "Не найдено допустимых адресов камеры." << std::endl;
        else {
            std::cout << "Найдены допустимые адреса камеры (показаны первые 3):" << std::endl;
            for (size_t i = 0; i < std::min(validAddresses.size(), size_t(3)); i++) {
                std::cout << "0x" << std::hex << validAddresses[i] << " ";
            }
            std::cout << std::endl;
        }
        return validAddresses;
    }
    return addresses;
}

CDebugCamera::CDebugCamera() {
    // Дополнительная инициализация, если требуется.
}

extern "C" {

    DEBUGCAMERA_API int fnDebugCamera(void) {
        return 0;
    }

    DEBUGCAMERA_API DWORD WINAPI MainThread(LPVOID lpParam) {
        static bool initialized = false;
        if (initialized) {
            return 0;
        }
        initialized = true;

        if (!ConsoleUtils::Logger().Initialize("Debug Console for DebugCamera")) {
            ConsoleUtils::LogError("Не удалось инициализировать консоль");
            return 1;
        }

        ConsoleUtils::LogSuccess("========================================");
        ConsoleUtils::LogSuccess("   ОТЛАДКА КАМЕРЫ DARK SOULS REMASTERED   ");
        ConsoleUtils::LogSuccess("========================================");

        // Запуск интерактивного меню
        ConsoleUI::ConsoleMenu menu;
        menu.Run();

        ConsoleUtils::LogSuccess("Выход из программы");
        return 0;
    }

} // extern "C"
