#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::vpk {

struct VpkEntry {
    std::string path;
    std::uint32_t crc32 = 0;
    std::uint16_t archiveIndex = 0;
    std::uint32_t offset = 0;
    std::uint32_t length = 0;
    std::vector<std::uint8_t> preloadBytes;
};

} // namespace vmsourceconv::vpk
