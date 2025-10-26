#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "MemoryManager.h"

namespace ConsoleUI {

    struct CommandContext {
        std::vector<uintptr_t> cameraAddresses;
        std::vector<uintptr_t> playerAddresses;
        uintptr_t selectedCameraAddress;
        uintptr_t selectedPlayerAddress;
        std::string currentPattern;
        std::string currentMask;
    };

    class ConsoleMenu {
    public:
        ConsoleMenu();
        ~ConsoleMenu();

        void Run();
        void DisplayMenu();
        void ProcessCommand(const std::string& command);

    private:
        CommandContext context;

        void CmdShowPos();
        void CmdSetPos();
        void CmdAddPos();
        void CmdPattern();
        void CmdShowPattern();
        void CmdScan();
        void CmdAutoScan();
        void CmdHelp();
        void CmdList();

        void DisplayHelpMessage();
    };
}
