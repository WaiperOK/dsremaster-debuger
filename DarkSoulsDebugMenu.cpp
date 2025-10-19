
#include "pch.h"
#include "DarkSoulsDebugMenu.h"
#include <windows.h>
#undef min
#undef max

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Запускаем основной поток при загрузке DLL.
        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        break;
    }
    return TRUE;
}
