
#include "pch.h"
#include "MemoryManager.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include "StringUtils.h"
#include <iostream>
#include <cstring>
#include <cmath>

#undef min
#undef max

bool WriteCameraCoordinates(uintptr_t address, const CameraCoordinates& coords) {
    if (!MemoryUtils::IsPageAccessible(address, sizeof(float) * 3)) {
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
    CameraCoordinates coords = { 0.0f, 0.0f, 0.0f };

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

    const float values[3] = { coords.x, coords.y, coords.z };
    bool hasSignal = false;
    for (float value : values) {
        // NaN and infinity compare false against everything, so reject them explicitly.
        if (!std::isfinite(value) || std::abs(value) >= MAX_COORD) {
            return false;
        }
        if (std::abs(value) > MIN_DEVIATION) {
            hasSignal = true;
        }
    }

    // A real camera may well sit at 0 on one axis; only an all zero triple is treated as empty.
    return hasSignal;
}
