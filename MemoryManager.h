#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>

struct CameraCoordinates {
    float x;
    float y;
    float z;
};

bool WriteCameraCoordinates(uintptr_t address, const CameraCoordinates& coords);
CameraCoordinates ReadCameraCoordinates(uintptr_t address);
bool AddCameraCoordinates(uintptr_t address, const CameraCoordinates& offset);
bool IsValidCameraAddress(uintptr_t address);
