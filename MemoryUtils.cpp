#include "pch.h"
#include "MemoryUtils.h"
#include <iostream>

namespace MemoryUtils {

    MemoryProtection::~MemoryProtection() {
        if (oldProtect != 0) {
            DWORD temp;
            RestorePageProtection(address, size, oldProtect);
        }
    }

    bool IsPageAccessible(uintptr_t address, size_t size) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<LPVOID>(address), &mbi, sizeof(mbi)) == 0)
            return false;

        if (mbi.State != MEM_COMMIT)
            return false;

        if ((mbi.Protect & PAGE_READONLY) || (mbi.Protect & PAGE_READWRITE) ||
            (mbi.Protect & PAGE_EXECUTE_READ) || (mbi.Protect & PAGE_EXECUTE_READWRITE)) {
            return true;
        }

        return false;
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

    bool SafeReadMemory(uintptr_t address, void* buffer, size_t size) {
        if (!buffer || size == 0) {
            std::cerr << "[-] Ошибка: неверные параметры для чтения" << std::endl;
            return false;
        }

        if (!IsPageAccessible(address, size)) {
            std::cerr << "[-] Памяти недоступна для чтения: 0x" << std::hex << address << std::endl;
            return false;
        }

        try {
            memcpy(buffer, reinterpret_cast<void*>(address), size);
            return true;
        } catch (...) {
            std::cerr << "[-] Исключение при чтении памяти: 0x" << std::hex << address << std::endl;
            return false;
        }
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

        try {
            memcpy(reinterpret_cast<void*>(address), buffer, size);
            RestorePageProtection(address, size, oldProtect);
            return true;
        } catch (...) {
            std::cerr << "[-] Исключение при записи памяти: 0x" << std::hex << address << std::endl;
            RestorePageProtection(address, size, oldProtect);
            return false;
        }
    }
}
