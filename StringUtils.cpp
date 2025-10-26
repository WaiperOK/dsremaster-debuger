#include "pch.h"
#include "StringUtils.h"
#include <sstream>
#include <cctype>
#include <algorithm>
#include <cstdlib>

namespace StringUtils {
    std::string Trim(const std::string& str) {
        const std::string whitespace = " \t\n\r";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos)
            return "";
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    bool IsValidHex(const std::string& str) {
        if (str.empty())
            return false;
        for (char c : str) {
            if (!std::isxdigit(static_cast<unsigned char>(c)))
                return false;
        }
        return true;
    }

    bool HexStringToAddress(const std::string& hexString, uintptr_t& address) {
        std::string str = Trim(hexString);
        if (str.size() >= 2 && (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X"))
            str = str.substr(2);
        if (!IsValidHex(str))
            return false;
        std::stringstream ss;
        ss << std::hex << str;
        ss >> address;
        return true;
    }

    std::string HexStringToBytes(const std::string& hexStr) {
        std::string bytes;
        if (hexStr.length() % 2 != 0)
            return "";
        for (size_t i = 0; i < hexStr.length(); i += 2) {
            std::string byteString = hexStr.substr(i, 2);
            char byte = static_cast<char>(strtol(byteString.c_str(), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
}
