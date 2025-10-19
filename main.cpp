#include "pch.h"           // Если используется precompiled header
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>

// Функция для удаления пробелов в начале и конце строки
std::string Trim(const std::string& str) {
    const std::string whitespace = " \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

int main() {
    while (true) {
        std::string input;
        std::cout << "Enter command ('showpos', 'setpos', 'addpos', 'pattern', 'showpattern', 'scan' or 'exit'): ";
        if (!std::getline(std::cin, input)) {
            std::cout << "No input received. Retrying..." << std::endl;
            Sleep(100); // Задержка 100 мс
            continue;
        }
        std::string trimmed = Trim(input);

        // Диагностический вывод полученной строки
        std::cout << "Input: \"" << trimmed << "\"" << std::endl;

        if (trimmed == "exit")
            break;

        if (trimmed.empty()) {
            std::cout << "Empty input, please type a command." << std::endl;
            Sleep(100);
            continue;
        }

        std::istringstream iss(trimmed);
        std::string command;
        iss >> command;

        // Обработка команд (пример обработки)
        if (command == "showpos") {
            std::cout << "Command 'showpos' received." << std::endl;
            // Здесь должен быть код для вывода позиции
        }
        else if (command == "setpos") {
            std::cout << "Command 'setpos' received." << std::endl;
            // Здесь должен быть код для установки новой позиции
        }
        else if (command == "addpos") {
            std::cout << "Command 'addpos' received." << std::endl;
            // Здесь должен быть код для добавления смещения
        }
        else if (command == "pattern") {
            std::cout << "Command 'pattern' received." << std::endl;
            // Здесь должен быть код для задания нового паттерна и маски
        }
        else if (command == "showpattern") {
            std::cout << "Command 'showpattern' received." << std::endl;
            // Здесь должен быть код для вывода текущего паттерна и маски
        }
        else if (command == "scan") {
            std::cout << "Command 'scan' received." << std::endl;
            // Здесь должен быть код для пересканирования памяти
        }
        else {
            std::cout << "Invalid command. Use 'showpos', 'setpos', 'addpos', 'pattern', 'showpattern', 'scan' or 'exit'." << std::endl;
        }
    }
    return 0;
}
