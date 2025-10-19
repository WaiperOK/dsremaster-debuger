
#include "pch.h"
#include "MemoryManager.h"
#include <windows.h>
#include <iostream>
#include <cstring>

#undef min
#undef max

void WriteCameraCoordinates(uintptr_t address, const CameraCoordinates& coords) {
    DWORD oldProtect;
    if (VirtualProtect(reinterpret_cast<LPVOID>(address), 12, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        try {
            float* ptr = reinterpret_cast<float*>(address);
            ptr[0] = coords.x;
            ptr[1] = coords.y;
            ptr[2] = coords.z;
        }
        catch (...) {
            std::cerr << "Failed to write coordinates at address: 0x" << std::hex << address << std::endl;
        }
        VirtualProtect(reinterpret_cast<LPVOID>(address), 12, oldProtect, &oldProtect);
    }
    else {
        std::cerr << "Failed to change memory protection at address: 0x"
            << std::hex << address << std::endl;
    }
}

CameraCoordinates ReadCameraCoordinates(uintptr_t address) {
    CameraCoordinates coords = { 0.0f, 0.0f, 0.0f, 0 };
    try {
        float* ptr = reinterpret_cast<float*>(address);
        coords.x = ptr[0];
        coords.y = ptr[1];
        coords.z = ptr[2];
        coords.magic = 0; // Не используем magic
    }
    catch (...) {
        std::cerr << "Failed to read coordinates at address: 0x" << std::hex << address << std::endl;
    }
    return coords;
}

void AddCameraCoordinates(uintptr_t address, const CameraCoordinates& offset) {
    CameraCoordinates coords = ReadCameraCoordinates(address);
    coords.x += offset.x;
    coords.y += offset.y;
    coords.z += offset.z;
    WriteCameraCoordinates(address, coords);
}

bool IsValidCameraAddress(uintptr_t address) {
    try {
        float* ptr = reinterpret_cast<float*>(address);
        // Проверяем, что значения находятся в разумных пределах для координат
        return (abs(ptr[0]) < 10000.0f && abs(ptr[1]) < 10000.0f && abs(ptr[2]) < 10000.0f &&
                ptr[0] != 0.0f && ptr[1] != 0.0f && ptr[2] != 0.0f);
    }
    catch (...) {
        return false;
    }
}
