#pragma once

#include "serialization/kv3/Kv3Value.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace vmsourceconv::serialization::kv3 {

struct Kv3DecodeStatistics {
    std::size_t stringCount = 0;
    std::size_t nodeCount = 0;
    std::size_t objectCount = 0;
    std::size_t arrayCount = 0;
    std::size_t binaryBlobCount = 0;
    std::size_t maximumDepth = 0;
};

struct Kv3Document {
    std::array<std::uint8_t, 16> formatId{};
    Kv3Value root;
    Kv3DecodeStatistics statistics;
};

} // namespace vmsourceconv::serialization::kv3
