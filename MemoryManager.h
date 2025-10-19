#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>

// Структура для хранения координат камеры с дополнительным полем для проверки
struct CameraCoordinates {
    float x;
    float y;
    float z;
    int magic; // Ожидаемое значение: 0x12345678
};

// Объявления функций работы с памятью
void WriteCameraCoordinates(uintptr_t address, const CameraCoordinates& coords);
CameraCoordinates ReadCameraCoordinates(uintptr_t address);
void AddCameraCoordinates(uintptr_t address, const CameraCoordinates& offset);

// Функция для проверки актуальности найденного адреса камеры
bool IsValidCameraAddress(uintptr_t address);
