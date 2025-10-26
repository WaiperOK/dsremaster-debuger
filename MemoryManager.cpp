
#include "pch.h"
#include "MemoryManager.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include <windows.h>
#include <iostream>
#include <cstring>
#include <cmath>

#undef min
#undef max

bool WriteCameraCoordinates(uintptr_t address, const CameraCoordinates& coords) {
    if (!MemoryUtils::IsPageAccessible(address, sizeof(CameraCoordinates))) {
        ConsoleUtils::LogError("Адрес памяти недоступен: 0x" + std::to_string(address));
        return false;
    }

    if (!MemoryUtils::SafeWriteMemory(address, &coords, sizeof(float) * 3)) {
        ConsoleUtils::LogError("Не удалось записать координаты: 0x" + std::to_string(address));
        return false;
    }

    return true;
}

CameraCoordinates ReadCameraCoordinates(uintptr_t address) {
    CameraCoordinates coords = { 0.0f, 0.0f, 0.0f, 0 };

    if (!MemoryUtils::IsPageAccessible(address, sizeof(float) * 3)) {
        ConsoleUtils::LogError("Адрес памяти недоступен для чтения: 0x" + std::to_string(address));
        return coords;
    }

    if (!MemoryUtils::SafeReadMemory(address, &coords, sizeof(float) * 3)) {
        ConsoleUtils::LogError("Не удалось прочитать координаты: 0x" + std::to_string(address));
        return coords;
    }

    return coords;
}

bool AddCameraCoordinates(uintptr_t address, const CameraCoordinates& offset) {
    CameraCoordinates coords = ReadCameraCoordinates(address);
    coords.x += offset.x;
    coords.y += offset.y;
    coords.z += offset.z;
    return WriteCameraCoordinates(address, coords);
}

bool IsValidCameraAddress(uintptr_t address) {
    if (!MemoryUtils::IsPageAccessible(address, sizeof(float) * 3)) {
        return false;
    }

    CameraCoordinates coords = ReadCameraCoordinates(address);

    constexpr float MAX_COORD = 10000.0f;
    constexpr float MIN_DEVIATION = 0.001f;

    return (std::abs(coords.x) < MAX_COORD && std::abs(coords.y) < MAX_COORD && std::abs(coords.z) < MAX_COORD &&
            std::abs(coords.x) > MIN_DEVIATION && std::abs(coords.y) > MIN_DEVIATION && std::abs(coords.z) > MIN_DEVIATION);
}
