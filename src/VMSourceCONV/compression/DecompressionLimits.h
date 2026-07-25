#pragma once

#include <cstddef>

namespace vmsourceconv::compression {

struct DecompressionLimits {
    std::size_t maximumOutputBytes = 1024ULL * 1024ULL * 1024ULL;
};

} // namespace vmsourceconv::compression
