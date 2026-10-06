// Standalone checks for the platform independent helpers. They need no game and no Windows:
// See tools/README.md for how to build and run it.
#include <cstdio>
#include <string>
#include <vector>

#include "PatternMatch.h"
#include "StringUtils.h"

static int g_failures = 0;
#define CHECK(cond)                                                                     \
    do {                                                                                \
        if (!(cond)) {                                                                  \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                 \
            g_failures++;                                                               \
        }                                                                               \
    } while (0)

static std::vector<uintptr_t> Find(const std::vector<uint8_t>& data, const std::string& pattern,
                                   const std::string& mask, uintptr_t base = 0x1000) {
    std::vector<uintptr_t> out;
    PatternMatch::FindAll(data.data(), data.size(), base, pattern.data(), mask.c_str(),
                          mask.size(), out);
    return out;
}

int main() {
    // --- PatternMatch ---
    const std::vector<uint8_t> data = {0x00, 0x11, 0x22, 0x33, 0x11, 0x22, 0x44, 0x11, 0x22};
    CHECK(Find(data, "\x11\x22", "xx") == (std::vector<uintptr_t>{0x1001, 0x1004, 0x1007}));
    // The last possible position must be reported (the old loop skipped it).
    CHECK(Find(data, "\x11\x22", "xx").back() == 0x1007);
    // Wildcards.
    CHECK(Find(data, std::string("\x11\x00\x33", 3), "x?x") == (std::vector<uintptr_t>{0x1001}));
    // No match, pattern longer than the data, empty pattern.
    CHECK(Find(data, "\x99", "x").empty());
    CHECK(Find(data, std::string(20, 'a'), std::string(20, 'x')).empty());
    CHECK(Find(data, "", "").empty());
    // All wildcard pattern matches at every offset.
    CHECK(Find(data, std::string(3, '\0'), "???").size() == data.size() - 2);

    // --- StringUtils ---
    CHECK(StringUtils::HexStringToBytes("DEADBEEF") == std::string("\xDE\xAD\xBE\xEF", 4));
    CHECK(StringUtils::HexStringToBytes("deadbeef") == std::string("\xDE\xAD\xBE\xEF", 4));
    CHECK(StringUtils::HexStringToBytes("ABC").empty());  // odd length
    CHECK(StringUtils::HexStringToBytes("ZZ").empty());   // not hex: used to become a 0x00 byte
    CHECK(StringUtils::HexStringToBytes("1G").empty());
    CHECK(StringUtils::HexStringToBytes("").empty());
    CHECK(StringUtils::FormatAddress(0x1A2B3C) == "0x1A2B3C");
    CHECK(StringUtils::FormatAddress(0) == "0x0");
    uintptr_t address = 0;
    CHECK(StringUtils::HexStringToAddress("0x7FF612340000", address) && address == 0x7FF612340000);
    CHECK(!StringUtils::HexStringToAddress("0xZZ", address));

    std::printf(g_failures ? "FAILED (%d)\n" : "ALL OK\n", g_failures);
    return g_failures ? 1 : 0;
}
