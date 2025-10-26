#include "pch.h"
#include "ConsoleUI.h"
#include "StringUtils.h"
#include "MemoryUtils.h"
#include "ConsoleUtils.h"
#include "PatternScanner.h"
#include "AddressDetector.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <conio.h>

namespace ConsoleUI {

    ConsoleMenu::ConsoleMenu()
        : selectedCameraAddress(0), selectedPlayerAddress(0) {
        context.currentPattern = "\x00\x00\x80\x3F\x00\x00\x00\x40\x00\x00\x40\x40";
        context.currentMask = "xxxxxxxxxxxx";
    }

    ConsoleMenu::~ConsoleMenu() {
        ConsoleUtils::Logger().Cleanup();
    }

    void ConsoleMenu::Run() {
        DisplayMenu();

        while (true) {
            std::cout << "\n>>> ";
            std::cout.flush();

            std::string input;
            if (!std::getline(std::cin, input)) {
                Sleep(100);
                continue;
            }

            std::string trimmed = StringUtils::Trim(input);
            if (trimmed.empty()) {
                Sleep(100);
                continue;
            }

            if (trimmed == "exit") {
                ConsoleUtils::LogSuccess("Выход из программы");
                break;
            }

            ProcessCommand(trimmed);
        }
    }

    void ConsoleMenu::DisplayMenu() {
        ConsoleUtils::LogSuccess("Доступные команды:");
        std::cout << "  showpos    - показать координаты камеры" << std::endl;
        std::cout << "  setpos     - установить координаты камеры" << std::endl;
        std::cout << "  addpos     - добавить смещение к координатам" << std::endl;
        std::cout << "  pattern    - установить новый шаблон поиска" << std::endl;
        std::cout << "  showpattern - показать текущий шаблон" << std::endl;
        std::cout << "  scan       - сканировать с текущим шаблоном" << std::endl;
        std::cout << "  autoscan   - автоматическое многофазное сканирование" << std::endl;
        std::cout << "  list       - показать найденные адреса" << std::endl;
        std::cout << "  help       - справка" << std::endl;
        std::cout << "  exit       - выход" << std::endl;
    }

    void ConsoleMenu::ProcessCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        if (cmd == "showpos") {
            CmdShowPos();
        } else if (cmd == "setpos") {
            CmdSetPos();
        } else if (cmd == "addpos") {
            CmdAddPos();
        } else if (cmd == "pattern") {
            CmdPattern();
        } else if (cmd == "showpattern") {
            CmdShowPattern();
        } else if (cmd == "scan") {
            CmdScan();
        } else if (cmd == "autoscan") {
            CmdAutoScan();
        } else if (cmd == "list") {
            CmdList();
        } else if (cmd == "help") {
            CmdHelp();
        } else {
            ConsoleUtils::LogWarning("Неизвестная команда: " + cmd);
            DisplayMenu();
        }
    }

    void ConsoleMenu::CmdShowPos() {
        if (context.cameraAddresses.empty()) {
            ConsoleUtils::LogError("Адреса камеры не найдены. Используйте 'scan' или 'autoscan'");
            return;
        }

        uintptr_t addr = context.selectedCameraAddress ? context.selectedCameraAddress : context.cameraAddresses[0];

        if (!MemoryUtils::IsPageAccessible(addr, sizeof(float) * 3)) {
            ConsoleUtils::LogError("Адрес недоступен: 0x" + std::to_string(addr));
            return;
        }

        CameraCoordinates coords = ReadCameraCoordinates(addr);
        ConsoleUtils::LogSuccess("Координаты камеры:");
        std::cout << std::fixed << std::setprecision(2)
                  << "  X: " << coords.x << std::endl
                  << "  Y: " << coords.y << std::endl
                  << "  Z: " << coords.z << std::endl;
    }

    void ConsoleMenu::CmdSetPos() {
        if (context.cameraAddresses.empty()) {
            ConsoleUtils::LogError("Адреса камеры не найдены");
            return;
        }

        uintptr_t addr = context.selectedCameraAddress ? context.selectedCameraAddress : context.cameraAddresses[0];

        ConsoleUtils::LogInfo("Введите координаты (X Y Z): ");
        float x, y, z;
        if (!(std::cin >> x >> y >> z)) {
            ConsoleUtils::LogError("Ошибка ввода");
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            return;
        }

        CameraCoordinates coords = { x, y, z };
        if (WriteCameraCoordinates(addr, coords)) {
            ConsoleUtils::LogSuccess("Координаты установлены");
        } else {
            ConsoleUtils::LogError("Не удалось установить координаты");
        }
    }

    void ConsoleMenu::CmdAddPos() {
        if (context.cameraAddresses.empty()) {
            ConsoleUtils::LogError("Адреса камеры не найдены");
            return;
        }

        uintptr_t addr = context.selectedCameraAddress ? context.selectedCameraAddress : context.cameraAddresses[0];

        ConsoleUtils::LogInfo("Введите смещение (X Y Z): ");
        float dx, dy, dz;
        if (!(std::cin >> dx >> dy >> dz)) {
            ConsoleUtils::LogError("Ошибка ввода");
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            return;
        }

        CameraCoordinates offset = { dx, dy, dz };
        if (AddCameraCoordinates(addr, offset)) {
            ConsoleUtils::LogSuccess("Смещение применено");
        } else {
            ConsoleUtils::LogError("Не удалось применить смещение");
        }
    }

    void ConsoleMenu::CmdPattern() {
        std::cout << "Введите новый шаблон в HEX (например 3F800000400000004040000078563412): ";
        std::string newHex;
        std::getline(std::cin, newHex);

        std::cout << "Введите новую маску (например xxxxxxxxxxxx): ";
        std::string newMask;
        std::getline(std::cin, newMask);

        newHex = StringUtils::Trim(newHex);
        newMask = StringUtils::Trim(newMask);

        std::string newPattern = StringUtils::HexStringToBytes(newHex);
        if (newPattern.empty() || newPattern.length() != newMask.length()) {
            ConsoleUtils::LogError("Неверный шаблон или маска");
            return;
        }

        context.currentPattern = newPattern;
        context.currentMask = newMask;
        ConsoleUtils::LogSuccess("Шаблон и маска установлены");
    }

    void ConsoleMenu::CmdShowPattern() {
        std::cout << "Текущий шаблон (HEX): ";
        for (unsigned char c : context.currentPattern) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)c << " ";
        }
        std::cout << std::endl;
        std::cout << "Текущая маска: " << context.currentMask << std::endl;
    }

    void ConsoleMenu::CmdScan() {
        if (context.currentPattern.empty()) {
            ConsoleUtils::LogError("Шаблон не установлен");
            return;
        }

        std::string hexPattern;
        for (unsigned char c : context.currentPattern) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", c);
            hexPattern += buf;
        }

        context.cameraAddresses = PatternScanner::FindCameraAddresses(hexPattern, context.currentMask);

        if (context.cameraAddresses.empty()) {
            ConsoleUtils::LogWarning("Адреса не найдены");
        } else {
            ConsoleUtils::LogSuccess("Найдено " + std::to_string(context.cameraAddresses.size()) + " адрес(ов)");
            if (!context.cameraAddresses.empty()) {
                context.selectedCameraAddress = context.cameraAddresses[0];
            }
        }
    }

    void ConsoleMenu::CmdAutoScan() {
        ConsoleUtils::LogInfo("Запуск автоматического сканирования...");
        GameAddresses gameAddresses = AddressDetector::AutoDetectAll();

        context.cameraAddresses = gameAddresses.cameraAddresses;
        context.playerAddresses = gameAddresses.playerAddresses;

        if (!context.cameraAddresses.empty()) {
            context.selectedCameraAddress = context.cameraAddresses[0];
        }
        if (!context.playerAddresses.empty()) {
            context.selectedPlayerAddress = context.playerAddresses[0];
        }

        ConsoleUtils::LogSuccess("Автосканирование завершено");
    }

    void ConsoleMenu::CmdList() {
        std::cout << "\nАдреса КАМЕРЫ (" << context.cameraAddresses.size() << "):" << std::endl;
        for (size_t i = 0; i < std::min(context.cameraAddresses.size(), size_t(5)); i++) {
            std::cout << "  [" << i << "] 0x" << std::hex << context.cameraAddresses[i] << std::dec << std::endl;
        }

        std::cout << "\nАдреса ИГРОКА (" << context.playerAddresses.size() << "):" << std::endl;
        for (size_t i = 0; i < std::min(context.playerAddresses.size(), size_t(5)); i++) {
            std::cout << "  [" << i << "] 0x" << std::hex << context.playerAddresses[i] << std::dec << std::endl;
        }
    }

    void ConsoleMenu::CmdHelp() {
        DisplayHelpMessage();
    }

    void ConsoleMenu::DisplayHelpMessage() {
        std::cout << "\n=== СПРАВКА ===" << std::endl;
        std::cout << "showpos    - Показать текущие координаты камеры" << std::endl;
        std::cout << "setpos     - Установить новые координаты (введите X Y Z)" << std::endl;
        std::cout << "addpos     - Добавить смещение (введите DX DY DZ)" << std::endl;
        std::cout << "pattern    - Установить шаблон поиска (HEX и маска)" << std::endl;
        std::cout << "showpattern - Показать текущий шаблон" << std::endl;
        std::cout << "scan       - Сканировать память с текущим шаблоном" << std::endl;
        std::cout << "autoscan   - Многофазное сканирование (определяет адреса автоматически)" << std::endl;
        std::cout << "list       - Показать найденные адреса" << std::endl;
        std::cout << "exit       - Выход из программы" << std::endl;
    }
}
