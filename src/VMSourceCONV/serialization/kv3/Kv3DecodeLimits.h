#pragma once

#include "compression/DecompressionLimits.h"

#include <cstddef>

namespace vmsourceconv::serialization::kv3 {

struct Kv3DecodeLimits {
    compression::DecompressionLimits decompression;
    std::size_t maximumNodes = 10'000'000;
    std::size_t maximumDepth = 512;
    std::size_t maximumStrings = 5'000'000;
    std::size_t maximumStringBytes = 512ULL * 1024ULL * 1024ULL;
};

} // namespace vmsourceconv::serialization::kv3
