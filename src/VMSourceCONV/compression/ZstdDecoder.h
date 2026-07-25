#pragma once

#include "compression/DecompressionLimits.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vmsourceconv::compression {

class ZstdDecoder final {
public:
    [[nodiscard]] std::vector<std::uint8_t> Decode(
        const std::uint8_t* input,
        std::size_t inputSize,
        std::size_t outputSize,
        const DecompressionLimits& limits = {}) const;
};

} // namespace vmsourceconv::compression
