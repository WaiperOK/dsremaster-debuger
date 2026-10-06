#include "pch.h"
#include <windows.h>
#include <iostream>
#include "DebugCamera.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // The per thread attach/detach notifications are not used.
        DisableThreadLibraryCalls(hModule);
        if (HANDLE thread = CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr)) {
            // The handle is never used again, so do not leak it.
            CloseHandle(thread);
        }
        break;
    }
    return TRUE;
}
