#pragma once

#include <cstdint>
#include <windows.h>

namespace MemoryUtils {

    struct MemoryProtection {
        DWORD oldProtect;
        uintptr_t address;
        size_t size;

        MemoryProtection(uintptr_t addr, size_t sz) : address(addr), size(sz), oldProtect(0) {}
        ~MemoryProtection();
    };

    bool IsPageAccessible(uintptr_t address, size_t size);
    bool IsValidAddress(uintptr_t address);
    bool ChangePageProtection(uintptr_t address, size_t size, DWORD newProtect, DWORD& oldProtect);
    bool RestorePageProtection(uintptr_t address, size_t size, DWORD oldProtect);

    bool SafeReadMemory(uintptr_t address, void* buffer, size_t size);
    bool SafeWriteMemory(uintptr_t address, const void* buffer, size_t size);
}
