#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct GameAddresses {
    std::vector<uintptr_t> playerAddresses;
    std::vector<uintptr_t> cameraAddresses;
};

namespace AddressDetector {
    GameAddresses AutoDetectAll();
    std::vector<uintptr_t> AutoDetectCameraAddresses();
}
