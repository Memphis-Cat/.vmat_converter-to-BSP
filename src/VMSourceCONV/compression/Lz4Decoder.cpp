#include "compression/Lz4Decoder.h"

#include "compression/CompressionError.h"

#include <lz4.h>

#include <limits>
#include <memory>
#include <string>

namespace vmsourceconv::compression {
namespace {

int CheckedInt(const std::size_t value, const char* name) {
    if (value > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw CompressionError(std::string(name) + " exceeds LZ4's int range");
    }
    return static_cast<int>(value);
}

void ValidateOutputSize(
    const std::size_t outputSize,
    const DecompressionLimits& limits) {
    if (outputSize > limits.maximumOutputBytes) {
        throw CompressionError("LZ4 output exceeds the configured allocation limit");
    }
}

} // namespace

std::vector<std::uint8_t> Lz4Decoder::DecodeBlock(
    const std::uint8_t* input,
    const std::size_t inputSize,
    const std::size_t outputSize,
    const DecompressionLimits& limits) const {
    ValidateOutputSize(outputSize, limits);
    if (input == nullptr && inputSize != 0) {
        throw CompressionError("LZ4 input pointer is null");
    }

    std::vector<std::uint8_t> output(outputSize);
    if (outputSize == 0) {
        if (inputSize != 0) {
            throw CompressionError("non-empty LZ4 input has an empty output size");
        }
        return output;
    }

    const auto decoded = LZ4_decompress_safe(
        reinterpret_cast<const char*>(input),
        reinterpret_cast<char*>(output.data()),
        CheckedInt(inputSize, "LZ4 input size"),
        CheckedInt(outputSize, "LZ4 output size"));

    if (decoded < 0) {
        throw CompressionError("LZ4 block is malformed");
    }
    if (decoded != CheckedInt(outputSize, "LZ4 output size")) {
        throw CompressionError(
            "LZ4 produced " + std::to_string(decoded)
            + " bytes, expected " + std::to_string(outputSize));
    }

    return output;
}

std::vector<std::uint8_t> Lz4Decoder::DecodeChainedBlocks(
    const std::vector<std::vector<std::uint8_t>>& blocks,
    const std::vector<std::size_t>& decodedBlockSizes,
    const std::size_t outputSize,
    const DecompressionLimits& limits) const {
    ValidateOutputSize(outputSize, limits);
    if (blocks.size() != decodedBlockSizes.size()) {
        throw CompressionError("LZ4 chained block metadata is inconsistent");
    }

    std::unique_ptr<LZ4_streamDecode_t, void(*)(LZ4_streamDecode_t*)> stream(
        LZ4_createStreamDecode(),
        [](LZ4_streamDecode_t* value) {
            if (value != nullptr) {
                LZ4_freeStreamDecode(value);
            }
        });
    if (!stream) {
        throw CompressionError("unable to allocate an LZ4 decode stream");
    }
    if (LZ4_setStreamDecode(stream.get(), nullptr, 0) == 0) {
        throw CompressionError("unable to initialize an LZ4 decode stream");
    }

    std::vector<std::uint8_t> output(outputSize);
    std::size_t outputOffset = 0;

    for (std::size_t index = 0; index < blocks.size(); ++index) {
        const auto expected = decodedBlockSizes[index];
        if (expected > outputSize - outputOffset) {
            throw CompressionError("LZ4 chained output exceeds its destination");
        }

        const auto& block = blocks[index];
        const auto decoded = LZ4_decompress_safe_continue(
            stream.get(),
            reinterpret_cast<const char*>(block.data()),
            reinterpret_cast<char*>(output.data() + outputOffset),
            CheckedInt(block.size(), "LZ4 chained input size"),
            CheckedInt(expected, "LZ4 chained output size"));

        if (decoded < 0) {
            throw CompressionError("an LZ4 chained block is malformed");
        }
        if (decoded != CheckedInt(expected, "LZ4 chained output size")) {
            throw CompressionError("LZ4 chained block produced an unexpected size");
        }
        outputOffset += expected;
    }

    if (outputOffset != outputSize) {
        throw CompressionError("LZ4 chained blocks did not fill the destination");
    }
    return output;
}

} // namespace vmsourceconv::compression
