#pragma once

#include <iostream>
#include <string>
#include <windows.h>

namespace ConsoleUtils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR_LEVEL,
    SUCCESS
};

class ConsoleLogger {
public:
    static ConsoleLogger& Instance();

    bool Initialize(const std::string& title);
    void SetLogLevel(LogLevel level);
    void Log(LogLevel level, const std::string& message);
    void Cleanup();

    void SetColor(int colorCode);
    void ResetColor();

private:
    ConsoleLogger();
    ~ConsoleLogger();

    HANDLE hConsole;
    LogLevel minLogLevel;
    bool initialized;

    void LogToConsole(LogLevel level, const std::string& message);
    std::string GetLevelPrefix(LogLevel level);
    int GetColorForLevel(LogLevel level);
};

inline ConsoleLogger& Logger() {
    return ConsoleLogger::Instance();
}

void InitializeConsole(const std::string& title);
void CleanupConsole();
void LogDebug(const std::string& message);
void LogInfo(const std::string& message);
void LogWarning(const std::string& message);
void LogError(const std::string& message);
void LogSuccess(const std::string& message);

} // namespace ConsoleUtils
