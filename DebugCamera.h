#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#ifdef DEBUGCAMERA_EXPORTS
#define DEBUGCAMERA_API __declspec(dllexport)
#else
#define DEBUGCAMERA_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // Прототипы экспортируемых функций (с C‑линкингом)
    DEBUGCAMERA_API DWORD WINAPI MainThread(LPVOID lpParam);
    DEBUGCAMERA_API int fnDebugCamera(void);

#ifdef __cplusplus
}
#endif

// Экспортированный класс (с C++‑линкингом)
class DEBUGCAMERA_API CDebugCamera {
public:
    CDebugCamera();
    // Дополнительные методы можно добавить здесь.
};
