#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace StringUtils {
    std::string Trim(const std::string& str);
    bool IsValidHex(const std::string& str);
    bool HexStringToAddress(const std::string& hexString, uintptr_t& address);
    std::string HexStringToBytes(const std::string& hexStr);
    // Formats an address as 0x1A2B3C (upper case hex).
    std::string FormatAddress(uintptr_t address);
}
