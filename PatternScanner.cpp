#include "pch.h"
#include "PatternScanner.h"
#include "StringUtils.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include "MemoryManager.h"
#include "PatternMatch.h"
#include <psapi.h>
#include <algorithm>

namespace PatternScanner {

    // Structured exception handling cannot share a function with C++ objects that need
    // unwinding, so the scan of one readable span lives in its own small function.
    static bool ScanSpan(uintptr_t start, uintptr_t end, const char* pattern, const char* mask,
                         size_t length, std::vector<uintptr_t>& out) {
        __try {
            PatternMatch::FindAll(reinterpret_cast<const uint8_t*>(start), end - start, start,
                                  pattern, mask, length, out);
            return true;
        } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
                            GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR
                        ? EXCEPTION_EXECUTE_HANDLER
                        : EXCEPTION_CONTINUE_SEARCH) {
            return false;
        }
    }

    std::vector<uintptr_t> FindAllPatterns(HMODULE hModule, const char* pattern, const char* mask) {
        std::vector<uintptr_t> addresses;
        MODULEINFO moduleInfo;

        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
            ConsoleUtils::LogError("Не удалось получить информацию о модуле");
            return addresses;
        }

        const uintptr_t startAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
        const uintptr_t endAddress = startAddress + moduleInfo.SizeOfImage;
        const size_t patternLength = strlen(mask);

        // Walk the image region by region: one VirtualQuery per region instead of one per byte,
        // and adjacent readable regions are merged so a match crossing a boundary is not missed.
        uintptr_t spanStart = 0;
        uintptr_t spanEnd = 0;
        const auto flush = [&]() {
            if (spanEnd > spanStart) {
                if (!ScanSpan(spanStart, spanEnd, pattern, mask, patternLength, addresses)) {
                    ConsoleUtils::LogWarning("Область памяти стала недоступна во время сканирования");
                }
            }
            spanStart = spanEnd = 0;
        };

        uintptr_t current = startAddress;
        while (current < endAddress) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)) == 0)
                break;

            const uintptr_t regionStart = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            const uintptr_t regionEnd = std::min(regionStart + mbi.RegionSize, endAddress);

            if (MemoryUtils::IsReadableRegion(mbi)) {
                if (spanEnd == 0) {
                    spanStart = current;
                }
                spanEnd = regionEnd;
            } else {
                flush();
            }
            current = regionEnd;
        }
        flush();

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
            return validAddresses;
        }

        ConsoleUtils::LogSuccess("Найдено " + std::to_string(validAddresses.size()) + " адрес(ов) камеры");
        return validAddresses;
    }
}
