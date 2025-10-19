#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#ifdef DARKSOULSDEBUGMENU_EXPORTS
#define DARKSOULSDEBUGMENU_API __declspec(dllexport)
#else
#define DARKSOULSDEBUGMENU_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

    DARKSOULSDEBUGMENU_API DWORD WINAPI MainThread(LPVOID lpParam);
    DARKSOULSDEBUGMENU_API int fnDarkSoulsDebugMenu(void);

#ifdef __cplusplus
}
#endif

// Экспортированный класс.
class DARKSOULSDEBUGMENU_API CDarkSoulsDebugMenu {
public:
    CDarkSoulsDebugMenu();
    // Дополнительные методы можно добавить здесь.
};
