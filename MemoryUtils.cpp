#include "pch.h"
#include "MemoryUtils.h"
#include <iostream>

namespace MemoryUtils {

    MemoryProtection::~MemoryProtection() {
        if (oldProtect != 0) {
            RestorePageProtection(address, size, oldProtect);
        }
    }

    bool IsReadableRegion(const MEMORY_BASIC_INFORMATION& mbi) {
        if (mbi.State != MEM_COMMIT)
            return false;

        // Guard pages raise an exception on the first access, no access pages always do.
        if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
            return false;

        constexpr DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
                                   PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                                   PAGE_EXECUTE_WRITECOPY;
        return (mbi.Protect & readable) != 0;
    }

    bool IsPageAccessible(uintptr_t address, size_t size) {
        if (size == 0)
            size = 1;
        if (address + size < address)
            return false;

        // A range can span several regions with different protections, so every one of them
        // has to be readable, not just the one holding the first byte.
        const uintptr_t end = address + size;
        uintptr_t current = address;
        while (current < end) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<LPCVOID>(current), &mbi, sizeof(mbi)) == 0)
                return false;
            if (!IsReadableRegion(mbi))
                return false;
            current = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        }
        return true;
    }

    bool IsValidAddress(uintptr_t address) {
        if (address < 0x10000 || address > 0x7FFFFFFF0000ULL)
            return false;

        return IsPageAccessible(address, 1);
    }

    bool ChangePageProtection(uintptr_t address, size_t size, DWORD newProtect, DWORD& oldProtect) {
        if (!IsPageAccessible(address, size)) {
            std::cerr << "[-] Страница памяти недоступна: 0x" << std::hex << address << std::endl;
            return false;
        }

        if (VirtualProtect(reinterpret_cast<LPVOID>(address), size, newProtect, &oldProtect) == 0) {
            DWORD error = GetLastError();
            std::cerr << "[-] Ошибка VirtualProtect: 0x" << std::hex << address
                      << " (код: " << std::dec << error << ")" << std::endl;
            return false;
        }

        return true;
    }

    bool RestorePageProtection(uintptr_t address, size_t size, DWORD oldProtect) {
        DWORD temp;
        if (VirtualProtect(reinterpret_cast<LPVOID>(address), size, oldProtect, &temp) == 0) {
            std::cerr << "[-] Ошибка восстановления защиты: 0x" << std::hex << address << std::endl;
            return false;
        }
        return true;
    }

    // A C++ catch (...) does not catch access violations in MSVC unless /EHa is used, so a page
    // that goes away between the accessibility check and the copy would crash the game.
    // Structured exception handling does, and it must live in a function without C++ objects.
    static bool GuardedCopy(void* destination, const void* source, size_t size) {
        __try {
            memcpy(destination, source, size);
            return true;
        } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
                            GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR
                        ? EXCEPTION_EXECUTE_HANDLER
                        : EXCEPTION_CONTINUE_SEARCH) {
            return false;
        }
    }

    bool SafeReadMemory(uintptr_t address, void* buffer, size_t size) {
        if (!buffer || size == 0) {
            std::cerr << "[-] Ошибка: неверные параметры для чтения" << std::endl;
            return false;
        }

        if (!IsPageAccessible(address, size)) {
            std::cerr << "[-] Памяти недоступна для чтения: 0x" << std::hex << address << std::endl;
            return false;
        }

        if (!GuardedCopy(buffer, reinterpret_cast<const void*>(address), size)) {
            std::cerr << "[-] Исключение при чтении памяти: 0x" << std::hex << address << std::endl;
            return false;
        }
        return true;
    }

    bool SafeWriteMemory(uintptr_t address, const void* buffer, size_t size) {
        if (!buffer || size == 0) {
            std::cerr << "[-] Ошибка: неверные параметры для записи" << std::endl;
            return false;
        }

        if (!IsPageAccessible(address, size)) {
            std::cerr << "[-] Памяти недоступна для записи: 0x" << std::hex << address << std::endl;
            return false;
        }

        DWORD oldProtect;
        if (!ChangePageProtection(address, size, PAGE_EXECUTE_READWRITE, oldProtect)) {
            return false;
        }

        const bool copied = GuardedCopy(reinterpret_cast<void*>(address), buffer, size);
        RestorePageProtection(address, size, oldProtect);
        if (!copied) {
            std::cerr << "[-] Исключение при записи памяти: 0x" << std::hex << address << std::endl;
            return false;
        }
        return true;
    }
}
