#include "pch.h"
#include "PatternScanner.h"
#include "StringUtils.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include "MemoryManager.h"
#include <psapi.h>
#include <algorithm>

namespace PatternScanner {

    std::vector<uintptr_t> FindAllPatterns(HMODULE hModule, const char* pattern, const char* mask) {
        std::vector<uintptr_t> addresses;
        MODULEINFO moduleInfo;

        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
            ConsoleUtils::LogError("Не удалось получить информацию о модуле");
            return addresses;
        }

        uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
        uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
        size_t patternLength = strlen(mask);

        for (uintptr_t current = startAddress; current < endAddress - patternLength; current++) {
            if (!MemoryUtils::IsPageAccessible(current, patternLength))
                continue;

            bool found = true;
            for (size_t i = 0; i < patternLength; i++) {
                if (mask[i] == 'x') {
                    try {
                        char byte = *reinterpret_cast<char*>(current + i);
                        if (byte != pattern[i]) {
                            found = false;
                            break;
                        }
                    } catch (...) {
                        found = false;
                        break;
                    }
                }
            }

            if (found)
                addresses.push_back(current);
        }

        return addresses;
    }

    std::vector<uintptr_t> FindCameraAddresses(const std::string& hexPattern, const std::string& mask) {
        HMODULE hModule = GetModuleHandle(nullptr);
        std::vector<uintptr_t> addresses;

        if (!hModule) {
            ConsoleUtils::LogError("Не удалось получить дескриптор модуля");
            return addresses;
        }

        std::string pattern = StringUtils::HexStringToBytes(hexPattern);
        if (pattern.empty() || pattern.length() != mask.length()) {
            ConsoleUtils::LogError("Неверный шаблон или маска");
            return addresses;
        }

        addresses = FindAllPatterns(hModule, pattern.c_str(), mask.c_str());

        if (addresses.empty()) {
            ConsoleUtils::LogWarning("Адреса не найдены по заданному шаблону");
            return addresses;
        }

        std::vector<uintptr_t> validAddresses;
        for (auto addr : addresses) {
            if (IsValidCameraAddress(addr))
                validAddresses.push_back(addr);
        }

        if (validAddresses.empty()) {
            ConsoleUtils::LogWarning("Валидные адреса не найдены");
            return addresses;
        }

        ConsoleUtils::LogSuccess("Найдено " + std::to_string(validAddresses.size()) + " адрес(ов) камеры");
        return validAddresses;
    }
}
