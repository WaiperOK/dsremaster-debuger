#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Platform independent byte pattern matching, kept free of Windows headers so it can be unit
// tested without the game. In a mask, 'x' means "this byte must match", anything else is a
// wildcard.
namespace PatternMatch {

    inline bool MatchAt(const uint8_t* data, const char* pattern, const char* mask, size_t length) {
        for (size_t i = 0; i < length; i++) {
            if (mask[i] == 'x' && data[i] != static_cast<uint8_t>(pattern[i]))
                return false;
        }
        return true;
    }

    // Appends the address (base + offset) of every match inside [data, data + size).
    inline void FindAll(const uint8_t* data, size_t size, uintptr_t base, const char* pattern,
                        const char* mask, size_t length, std::vector<uintptr_t>& out) {
        if (length == 0 || size < length)
            return;
        for (size_t i = 0; i + length <= size; i++) {
            if (MatchAt(data + i, pattern, mask, length))
                out.push_back(base + i);
        }
    }
}
