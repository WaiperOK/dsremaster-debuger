#include "pch.h"
#include <windows.h>
#include <iostream>
#include "DebugMenu.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;
    }
    return TRUE;
}
