#pragma once

#include "serialization/kv3/Kv3Compression.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

struct BinaryKv3Header {
    std::uint32_t magic = 0;
    int version = -1;
    std::array<std::uint8_t, 16> formatId{};
    std::uint32_t compressionMethod = 0;
    Kv3Compression compression = Kv3Compression::Unknown;
    std::uint16_t compressionDictionaryId = 0;
    std::uint16_t compressionFrameSize = 0;

    std::int32_t countBytes1 = 0;
    std::int32_t countBytes2 = 0;
    std::int32_t countBytes4 = 0;
    std::int32_t countBytes8 = 0;
    std::int32_t countTypes = 0;
    std::uint16_t countObjects = 0;
    std::uint16_t countArrays = 0;
    std::int32_t sizeUncompressedTotal = 0;
    std::int32_t sizeCompressedTotal = 0;
    std::int32_t countBlocks = 0;
    std::int32_t sizeBinaryBlobsBytes = 0;
    std::int32_t sizeBlockCompressedSizesBytes = 0;

    std::int32_t sizeUncompressedBuffer1 = 0;
    std::int32_t sizeCompressedBuffer1 = 0;
    std::int32_t sizeUncompressedBuffer2 = 0;
    std::int32_t sizeCompressedBuffer2 = 0;
    std::int32_t countBytes1Buffer2 = 0;
    std::int32_t countBytes2Buffer2 = 0;
    std::int32_t countBytes4Buffer2 = 0;
    std::int32_t countBytes8Buffer2 = 0;
    std::int32_t countObjectsBuffer2 = 0;
    std::int32_t countArraysBuffer2 = 0;

    std::size_t headerSize = 0;
    bool complete = false;
    std::vector<std::string> warnings;

    [[nodiscard]] std::string FormatIdString() const;
};

} // namespace vmsourceconv::serialization::kv3
