#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <windows.h>

namespace PatternScanner {

    std::vector<uintptr_t> FindAllPatterns(HMODULE hModule, const char* pattern, const char* mask);
    std::vector<uintptr_t> FindCameraAddresses(const std::string& hexPattern, const std::string& mask);
}
