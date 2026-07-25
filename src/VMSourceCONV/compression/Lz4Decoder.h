#pragma once

#include "compression/DecompressionLimits.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vmsourceconv::compression {

class Lz4Decoder final {
public:
    [[nodiscard]] std::vector<std::uint8_t> DecodeBlock(
        const std::uint8_t* input,
        std::size_t inputSize,
        std::size_t outputSize,
        const DecompressionLimits& limits = {}) const;

    [[nodiscard]] std::vector<std::uint8_t> DecodeChainedBlocks(
        const std::vector<std::vector<std::uint8_t>>& blocks,
        const std::vector<std::size_t>& decodedBlockSizes,
        std::size_t outputSize,
        const DecompressionLimits& limits = {}) const;
};

} // namespace vmsourceconv::compression
