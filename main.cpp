#include "pch.h"
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>
#include "StringUtils.h"

int main() {
    while (true) {
        std::string input;
        std::cout << "Enter command ('showpos', 'setpos', 'addpos', 'pattern', 'showpattern', 'scan' or 'exit'): ";
        if (!std::getline(std::cin, input)) {
            std::cout << "No input received. Retrying..." << std::endl;
            Sleep(100); // �������� 100 ��
            continue;
        }
        std::string trimmed = StringUtils::Trim(input);

        // ��������������� ����� ���������� ������
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

        // ��������� ������ (������ ���������)
        if (command == "showpos") {
            std::cout << "Command 'showpos' received." << std::endl;
            // ����� ������ ���� ��� ��� ������ �������
        }
        else if (command == "setpos") {
            std::cout << "Command 'setpos' received." << std::endl;
            // ����� ������ ���� ��� ��� ��������� ����� �������
        }
        else if (command == "addpos") {
            std::cout << "Command 'addpos' received." << std::endl;
            // ����� ������ ���� ��� ��� ���������� ��������
        }
        else if (command == "pattern") {
            std::cout << "Command 'pattern' received." << std::endl;
            // ����� ������ ���� ��� ��� ������� ������ �������� � �����
        }
        else if (command == "showpattern") {
            std::cout << "Command 'showpattern' received." << std::endl;
            // ����� ������ ���� ��� ��� ������ �������� �������� � �����
        }
        else if (command == "scan") {
            std::cout << "Command 'scan' received." << std::endl;
            // ����� ������ ���� ��� ��� ���������������� ������
        }
        else {
            std::cout << "Invalid command. Use 'showpos', 'setpos', 'addpos', 'pattern', 'showpattern', 'scan' or 'exit'." << std::endl;
        }
    }
    return 0;
}
