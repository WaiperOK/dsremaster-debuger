#include "pch.h"
#include "ConsoleUtils.h"
#include <sstream>
#include <chrono>
#include <iomanip>

namespace ConsoleUtils {

    ConsoleLogger::ConsoleLogger()
        : hConsole(NULL), minLogLevel(LogLevel::DEBUG), initialized(false) {}

    ConsoleLogger::~ConsoleLogger() {
        Cleanup();
    }

    ConsoleLogger& ConsoleLogger::Instance() {
        static ConsoleLogger instance;
        return instance;
    }

    bool ConsoleLogger::Initialize(const std::string& title) {
        if (initialized)
            return true;

        if (!AllocConsole()) {
            return false;
        }

        if (!SetConsoleTitleA(title.c_str())) {
            FreeConsole();
            return false;
        }

        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole == INVALID_HANDLE_VALUE) {
            FreeConsole();
            return false;
        }

        FILE* fpOut = nullptr;
        FILE* fpIn = nullptr;
        freopen_s(&fpOut, "CONOUT$", "w", stdout);
        freopen_s(&fpIn, "CONIN$", "r", stdin);

        DWORD mode = 0;
        if (GetConsoleMode(hConsole, &mode)) {
            SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }

        // Установка правильной кодировки для консоли (UTF-8)
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);

        // Включение поддержки ANSI escape sequences для цветного вывода
        if (GetConsoleMode(hConsole, &mode)) {
            SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }

        initialized = true;
        LogSuccess("Консоль инициализирована");
        return true;
    }

    void ConsoleLogger::SetLogLevel(LogLevel level) {
        minLogLevel = level;
    }

    std::string ConsoleLogger::GetLevelPrefix(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:       return "[DEBUG]";
            case LogLevel::INFO:        return "[INFO]";
            case LogLevel::WARNING:     return "[WARN]";
            case LogLevel::ERROR_LEVEL: return "[ERROR]";
            case LogLevel::SUCCESS:     return "[OK]";
            default:                    return "[LOG]";
        }
    }

    int ConsoleLogger::GetColorForLevel(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:       return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            case LogLevel::INFO:        return FOREGROUND_GREEN | FOREGROUND_BLUE;
            case LogLevel::WARNING:     return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            case LogLevel::ERROR_LEVEL: return FOREGROUND_RED | FOREGROUND_INTENSITY;
            case LogLevel::SUCCESS:     return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
            default:                    return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    }

    void ConsoleLogger::SetColor(int colorCode) {
        if (hConsole != NULL) {
            SetConsoleTextAttribute(hConsole, colorCode);
        }
    }

    void ConsoleLogger::ResetColor() {
        if (hConsole != NULL) {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
    }

    void ConsoleLogger::LogToConsole(LogLevel level, const std::string& message) {
        if (level < minLogLevel)
            return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm buf;
        localtime_s(&buf, &time);

        std::ostringstream oss;
        oss << std::put_time(&buf, "%H:%M:%S") << " " << GetLevelPrefix(level) << " " << message;

        SetColor(GetColorForLevel(level));
        std::cout << oss.str() << std::endl;
        ResetColor();
    }

    void ConsoleLogger::Log(LogLevel level, const std::string& message) {
        LogToConsole(level, message);
    }

    void ConsoleLogger::Cleanup() {
        if (initialized) {
            if (hConsole != NULL) {
                ResetColor();
            }
            initialized = false;
        }
    }

    void InitializeConsole(const std::string& title) {
        Logger().Initialize(title);
    }

    void CleanupConsole() {
        Logger().Cleanup();
    }

    void LogDebug(const std::string& message) {
        Logger().Log(LogLevel::DEBUG, message);
    }

    void LogInfo(const std::string& message) {
        Logger().Log(LogLevel::INFO, message);
    }

    void LogWarning(const std::string& message) {
        Logger().Log(LogLevel::WARNING, message);
    }

    void LogError(const std::string& message) {
        Logger().Log(LogLevel::ERROR_LEVEL, message);
    }

    void LogSuccess(const std::string& message) {
        Logger().Log(LogLevel::SUCCESS, message);
    }
}
