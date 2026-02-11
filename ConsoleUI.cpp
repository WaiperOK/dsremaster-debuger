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
#include <limits>

namespace ConsoleUI {

    ConsoleMenu::ConsoleMenu() {
        context.selectedCameraAddress = 0;
        context.selectedPlayerAddress = 0;
        context.currentPattern = "\x00\x00\x80\x3F\x00\x00\x00\x40\x00\x00\x40\x40";
        context.currentMask = "xxxxxxxxxxxx";
        context.moveSpeed = 1.0f;
        context.fov = 60.0f;
        context.timeScale = 1.0f;
        context.hudHidden = false;
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
        std::cout << "  1  showpos     - показать координаты камеры" << std::endl;
        std::cout << "  2  setpos      - установить координаты камеры" << std::endl;
        std::cout << "  3  addpos      - добавить смещение к координатам" << std::endl;
        std::cout << "  4  pattern     - установить новый шаблон поиска" << std::endl;
        std::cout << "  5  showpattern - показать текущий шаблон" << std::endl;
        std::cout << "  6  scan        - сканировать с текущим шаблоном" << std::endl;
        std::cout << "  7  autoscan    - автоматическое многофазное сканирование" << std::endl;
        std::cout << "  8  list        - показать найденные адреса" << std::endl;
        std::cout << "  9  help        - справка" << std::endl;
        std::cout << " 10  status      - текущее состояние debug menu" << std::endl;
        std::cout << " 11  speed       - установить скорость камеры" << std::endl;
        std::cout << " 12  fov         - установить FOV (30-140)" << std::endl;
        std::cout << " 13  timescale   - установить множитель времени (0.1-3.0)" << std::endl;
        std::cout << " 14  savepos     - сохранить текущую позицию как bookmark" << std::endl;
        std::cout << " 15  gotopos     - перейти к bookmark позиции" << std::endl;
        std::cout << " 16  bookmarks   - список bookmark-ов" << std::endl;
        std::cout << "  0  exit        - выход" << std::endl;
    }

    void ConsoleMenu::ProcessCommand(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;

        // Поддержка обоих форматов: имя команды или цифра
        if (cmd == "showpos" || cmd == "1") {
            CmdShowPos();
        } else if (cmd == "setpos" || cmd == "2") {
            CmdSetPos();
        } else if (cmd == "addpos" || cmd == "3") {
            CmdAddPos();
        } else if (cmd == "pattern" || cmd == "4") {
            CmdPattern();
        } else if (cmd == "showpattern" || cmd == "5") {
            CmdShowPattern();
        } else if (cmd == "scan" || cmd == "6") {
            CmdScan();
        } else if (cmd == "autoscan" || cmd == "7") {
            CmdAutoScan();
        } else if (cmd == "list" || cmd == "8") {
            CmdList();
        } else if (cmd == "help" || cmd == "9") {
            CmdHelp();
        } else if (cmd == "status" || cmd == "10") {
            CmdStatus();
        } else if (cmd == "speed" || cmd == "11") {
            CmdSpeed();
        } else if (cmd == "fov" || cmd == "12") {
            CmdFov();
        } else if (cmd == "timescale" || cmd == "13") {
            CmdTimeScale();
        } else if (cmd == "savepos" || cmd == "14") {
            CmdSavePos();
        } else if (cmd == "gotopos" || cmd == "15") {
            CmdGotoPos();
        } else if (cmd == "bookmarks" || cmd == "16") {
            CmdBookmarks();
        } else if (cmd == "exit" || cmd == "0") {
            ConsoleUtils::LogSuccess("Выход из программы");
            exit(0);
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
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

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
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

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

    void ConsoleMenu::CmdStatus() {
        std::cout << "\n=== DEBUG MENU STATUS ===" << std::endl;
        std::cout << "Speed: " << std::fixed << std::setprecision(2) << context.moveSpeed << std::endl;
        std::cout << "FOV: " << std::fixed << std::setprecision(2) << context.fov << std::endl;
        std::cout << "TimeScale: " << std::fixed << std::setprecision(2) << context.timeScale << std::endl;
        std::cout << "HUD: " << (context.hudHidden ? "hidden" : "visible") << std::endl;
        std::cout << "Bookmarks: " << context.bookmarks.size() << std::endl;
    }

    void ConsoleMenu::CmdSpeed() {
        ConsoleUtils::LogInfo("Введите новую скорость камеры (0.1 - 50.0): ");
        float value = 1.0f;
        if (!(std::cin >> value)) {
            ConsoleUtils::LogError("Ошибка ввода");
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (value < 0.1f || value > 50.0f) {
            ConsoleUtils::LogError("Скорость должна быть в диапазоне 0.1 - 50.0");
            return;
        }
        context.moveSpeed = value;
        ConsoleUtils::LogSuccess("Скорость обновлена");
    }

    void ConsoleMenu::CmdFov() {
        ConsoleUtils::LogInfo("Введите новый FOV (30 - 140): ");
        float value = 60.0f;
        if (!(std::cin >> value)) {
            ConsoleUtils::LogError("Ошибка ввода");
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (value < 30.0f || value > 140.0f) {
            ConsoleUtils::LogError("FOV должен быть в диапазоне 30 - 140");
            return;
        }
        context.fov = value;
        ConsoleUtils::LogSuccess("FOV обновлен");
    }

    void ConsoleMenu::CmdTimeScale() {
        ConsoleUtils::LogInfo("Введите новый множитель времени (0.1 - 3.0): ");
        float value = 1.0f;
        if (!(std::cin >> value)) {
            ConsoleUtils::LogError("Ошибка ввода");
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (value < 0.1f || value > 3.0f) {
            ConsoleUtils::LogError("TimeScale должен быть в диапазоне 0.1 - 3.0");
            return;
        }
        context.timeScale = value;
        ConsoleUtils::LogSuccess("TimeScale обновлен");
    }

    void ConsoleMenu::CmdSavePos() {
        if (context.cameraAddresses.empty()) {
            ConsoleUtils::LogError("Адреса камеры не найдены");
            return;
        }

        uintptr_t addr = context.selectedCameraAddress ? context.selectedCameraAddress : context.cameraAddresses[0];
        if (!MemoryUtils::IsPageAccessible(addr, sizeof(float) * 3)) {
            ConsoleUtils::LogError("Адрес недоступен");
            return;
        }

        CameraCoordinates coords = ReadCameraCoordinates(addr);
        ConsoleUtils::LogInfo("Введите имя bookmark: ");
        std::string name;
        if (!std::getline(std::cin, name)) {
            ConsoleUtils::LogError("Не удалось прочитать имя");
            return;
        }
        name = StringUtils::Trim(name);
        if (name.empty()) {
            ConsoleUtils::LogError("Имя bookmark не может быть пустым");
            return;
        }

        context.bookmarks[name] = coords;
        ConsoleUtils::LogSuccess("Позиция сохранена в bookmark '" + name + "'");
    }

    void ConsoleMenu::CmdGotoPos() {
        if (context.cameraAddresses.empty()) {
            ConsoleUtils::LogError("Адреса камеры не найдены");
            return;
        }

        ConsoleUtils::LogInfo("Введите имя bookmark: ");
        std::string name;
        if (!std::getline(std::cin, name)) {
            ConsoleUtils::LogError("Не удалось прочитать имя");
            return;
        }
        name = StringUtils::Trim(name);

        auto it = context.bookmarks.find(name);
        if (it == context.bookmarks.end()) {
            ConsoleUtils::LogError("Bookmark не найден: " + name);
            return;
        }

        uintptr_t addr = context.selectedCameraAddress ? context.selectedCameraAddress : context.cameraAddresses[0];
        if (WriteCameraCoordinates(addr, it->second)) {
            ConsoleUtils::LogSuccess("Переход к bookmark '" + name + "' выполнен");
        } else {
            ConsoleUtils::LogError("Не удалось применить bookmark");
        }
    }

    void ConsoleMenu::CmdBookmarks() {
        if (context.bookmarks.empty()) {
            ConsoleUtils::LogInfo("Bookmark-ов пока нет");
            return;
        }

        std::cout << "\n=== BOOKMARKS ===" << std::endl;
        for (const auto& [name, coords] : context.bookmarks) {
            std::cout << "  " << name << " -> ("
                      << std::fixed << std::setprecision(2)
                      << coords.x << ", " << coords.y << ", " << coords.z << ")" << std::endl;
        }
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
        std::cout << "status     - Показать текущее состояние debug menu" << std::endl;
        std::cout << "speed      - Установить скорость камеры" << std::endl;
        std::cout << "fov        - Установить FOV камеры" << std::endl;
        std::cout << "timescale  - Установить множитель времени" << std::endl;
        std::cout << "savepos    - Сохранить текущую позицию в bookmark" << std::endl;
        std::cout << "gotopos    - Перейти к сохраненному bookmark" << std::endl;
        std::cout << "bookmarks  - Показать список bookmark-ов" << std::endl;
        std::cout << "exit       - Выход из программы" << std::endl;
    }
}
