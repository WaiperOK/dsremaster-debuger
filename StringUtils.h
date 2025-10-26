#pragma once

#include <string>
#include <vector>

namespace StringUtils {
    std::string Trim(const std::string& str);
    bool IsValidHex(const std::string& str);
    bool HexStringToAddress(const std::string& hexString, uintptr_t& address);
    std::string HexStringToBytes(const std::string& hexStr);
}
