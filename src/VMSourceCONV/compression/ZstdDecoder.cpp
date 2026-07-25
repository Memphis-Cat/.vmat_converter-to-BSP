#include "compression/ZstdDecoder.h"

#include "compression/CompressionError.h"

#include <zstd.h>

#include <string>

namespace vmsourceconv::compression {

std::vector<std::uint8_t> ZstdDecoder::Decode(
    const std::uint8_t* input,
    const std::size_t inputSize,
    const std::size_t outputSize,
    const DecompressionLimits& limits) const {
    if (outputSize > limits.maximumOutputBytes) {
        throw CompressionError("Zstandard output exceeds the configured allocation limit");
    }
    if (input == nullptr && inputSize != 0) {
        throw CompressionError("Zstandard input pointer is null");
    }

    std::vector<std::uint8_t> output(outputSize);
    if (outputSize == 0) {
        if (inputSize != 0) {
            throw CompressionError("non-empty Zstandard input has an empty output size");
        }
        return output;
    }

    const auto decoded = ZSTD_decompress(
        output.data(),
        output.size(),
        input,
        inputSize);
    if (ZSTD_isError(decoded) != 0U) {
        throw CompressionError(
            std::string("Zstandard decode failed: ") + ZSTD_getErrorName(decoded));
    }
    if (decoded != outputSize) {
        throw CompressionError(
            "Zstandard produced " + std::to_string(decoded)
            + " bytes, expected " + std::to_string(outputSize));
    }

    return output;
}

} // namespace vmsourceconv::compression
