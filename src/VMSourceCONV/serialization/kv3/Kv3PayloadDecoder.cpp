#include "serialization/kv3/Kv3PayloadDecoder.h"

#include "compression/Lz4Decoder.h"
#include "compression/ZstdDecoder.h"
#include "serialization/kv3/Kv3Compression.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace vmsourceconv::serialization::kv3 {
namespace {

constexpr std::uint32_t Kv3Trailer = 0xFFEEDD00U;

class RawCursor final {
public:
    RawCursor(
        const std::vector<std::uint8_t>& bytes,
        const std::size_t offset,
        const std::size_t size)
        : bytes_(bytes), position_(offset) {
        if (offset > bytes.size() || size > bytes.size() - offset) {
            throw std::runtime_error("KV3 DATA range is outside the resource");
        }
        end_ = offset + size;
    }

    [[nodiscard]] std::size_t Remaining() const noexcept {
        return end_ - position_;
    }

    [[nodiscard]] std::vector<std::uint8_t> ReadBytes(const std::size_t size) {
        Require(size);
        std::vector<std::uint8_t> value(
            bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
            bytes_.begin() + static_cast<std::ptrdiff_t>(position_ + size));
        position_ += size;
        return value;
    }

    [[nodiscard]] std::uint32_t ReadU32() {
        Require(4);
        const auto value = static_cast<std::uint32_t>(bytes_[position_])
            | (static_cast<std::uint32_t>(bytes_[position_ + 1]) << 8U)
            | (static_cast<std::uint32_t>(bytes_[position_ + 2]) << 16U)
            | (static_cast<std::uint32_t>(bytes_[position_ + 3]) << 24U);
        position_ += 4;
        return value;
    }

private:
    void Require(const std::size_t size) const {
        if (size > end_ - position_) {
            throw std::runtime_error("KV3 compressed payload is truncated");
        }
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
    std::size_t end_ = 0;
};

[[nodiscard]] std::size_t CheckedSize(
    const std::int32_t value,
    const char* name) {
    if (value < 0) {
        throw std::runtime_error(std::string(name) + " is negative");
    }
    return static_cast<std::size_t>(value);
}

[[nodiscard]] std::size_t CheckedElementBytes(
    const std::int32_t count,
    const std::size_t elementSize,
    const char* name) {
    const auto value = CheckedSize(count, name);
    if (value > std::numeric_limits<std::size_t>::max() / elementSize) {
        throw std::runtime_error(std::string(name) + " overflows size_t");
    }
    return value * elementSize;
}

void Align(std::size_t& offset, const std::size_t alignment) {
    const auto mask = alignment - 1U;
    if (offset > std::numeric_limits<std::size_t>::max() - mask) {
        throw std::runtime_error("KV3 buffer alignment overflows size_t");
    }
    offset = (offset + mask) & ~mask;
}

void RequireRange(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::size_t size,
    const char* name) {
    if (offset > bytes.size() || size > bytes.size() - offset) {
        throw std::runtime_error(std::string(name) + " exceeds its decoded buffer");
    }
}

[[nodiscard]] std::vector<std::uint8_t> Slice(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::size_t size,
    const char* name) {
    RequireRange(bytes, offset, size, name);
    return {
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        bytes.begin() + static_cast<std::ptrdiff_t>(offset + size),
    };
}

[[nodiscard]] std::uint32_t ReadU32(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const char* name) {
    RequireRange(bytes, offset, 4, name);
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

[[nodiscard]] std::int32_t ReadI32(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const char* name) {
    return static_cast<std::int32_t>(ReadU32(bytes, offset, name));
}

[[nodiscard]] std::uint16_t ReadU16(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const char* name) {
    RequireRange(bytes, offset, 2, name);
    return static_cast<std::uint16_t>(bytes[offset])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8U);
}

[[nodiscard]] std::vector<std::uint8_t> DecodeBuffer(
    RawCursor& cursor,
    const Kv3Compression compressionMethod,
    const std::size_t compressedSize,
    const std::size_t uncompressedSize,
    const Kv3DecodeLimits& limits) {
    if (compressionMethod == Kv3Compression::None) {
        return cursor.ReadBytes(uncompressedSize);
    }

    const auto compressed = cursor.ReadBytes(compressedSize);
    if (compressionMethod == Kv3Compression::Lz4) {
        return compression::Lz4Decoder{}.DecodeBlock(
            compressed.data(),
            compressed.size(),
            uncompressedSize,
            limits.decompression);
    }
    if (compressionMethod == Kv3Compression::Zstd) {
        return compression::ZstdDecoder{}.Decode(
            compressed.data(),
            compressed.size(),
            uncompressedSize,
            limits.decompression);
    }

    throw std::runtime_error("unsupported KV3 compression method");
}

void PartitionAuxiliaryBuffer(
    const std::vector<std::uint8_t>& raw,
    const BinaryKv3Header& header,
    Kv3Payload& payload,
    const Kv3DecodeLimits& limits) {
    std::size_t offset = 0;

    const auto bytes1Size = CheckedElementBytes(header.countBytes1, 1, "countBytes1");
    payload.auxiliaryBuffer.bytes1 = Slice(raw, offset, bytes1Size, "KV3 auxiliary bytes1");
    offset += bytes1Size;

    if (header.countBytes2 > 0) {
        Align(offset, 2);
        const auto size = CheckedElementBytes(header.countBytes2, 2, "countBytes2");
        payload.auxiliaryBuffer.bytes2 = Slice(raw, offset, size, "KV3 auxiliary bytes2");
        offset += size;
    }

    if (header.countBytes4 > 0) {
        Align(offset, 4);
        const auto size = CheckedElementBytes(header.countBytes4, 4, "countBytes4");
        payload.auxiliaryBuffer.bytes4 = Slice(raw, offset, size, "KV3 auxiliary bytes4");
        offset += size;
    }

    if (header.countBytes8 > 0) {
        Align(offset, 8);
        const auto size = CheckedElementBytes(header.countBytes8, 8, "countBytes8");
        payload.auxiliaryBuffer.bytes8 = Slice(raw, offset, size, "KV3 auxiliary bytes8");
        offset += size;
    }

    if (offset != raw.size()) {
        throw std::runtime_error("KV3 auxiliary buffer has unexpected trailing bytes");
    }
    if (payload.auxiliaryBuffer.Remaining4() < 4) {
        throw std::runtime_error("KV3 auxiliary buffer does not contain a string count");
    }

    const auto stringCount = payload.auxiliaryBuffer.ReadI32();
    if (stringCount < 0) {
        throw std::runtime_error("KV3 string count is negative");
    }
    if (static_cast<std::size_t>(stringCount) > limits.maximumStrings) {
        throw std::runtime_error("KV3 string count exceeds the configured limit");
    }

    payload.strings.reserve(static_cast<std::size_t>(stringCount));
    for (std::int32_t index = 0; index < stringCount; ++index) {
        payload.strings.push_back(
            payload.auxiliaryBuffer.ReadCString(limits.maximumStringBytes));
    }
}

void PartitionMainBuffer(
    const std::vector<std::uint8_t>& raw,
    const BinaryKv3Header& header,
    Kv3Payload& payload,
    std::vector<std::uint8_t>& compressedBlobSizes) {
    std::size_t offset = 0;

    const auto objectLengthBytes = CheckedElementBytes(
        header.countObjectsBuffer2,
        4,
        "countObjectsBuffer2");
    RequireRange(raw, offset, objectLengthBytes, "KV3 object lengths");
    payload.objectLengths.reserve(static_cast<std::size_t>(header.countObjectsBuffer2));
    for (std::size_t index = 0; index < objectLengthBytes; index += 4) {
        payload.objectLengths.push_back(ReadI32(raw, offset + index, "KV3 object length"));
    }
    offset += objectLengthBytes;

    const auto bytes1Size = CheckedElementBytes(
        header.countBytes1Buffer2,
        1,
        "countBytes1Buffer2");
    payload.mainBuffer.bytes1 = Slice(raw, offset, bytes1Size, "KV3 main bytes1");
    offset += bytes1Size;

    if (header.countBytes2Buffer2 > 0) {
        Align(offset, 2);
        const auto size = CheckedElementBytes(
            header.countBytes2Buffer2,
            2,
            "countBytes2Buffer2");
        payload.mainBuffer.bytes2 = Slice(raw, offset, size, "KV3 main bytes2");
        offset += size;
    }

    if (header.countBytes4Buffer2 > 0) {
        Align(offset, 4);
        const auto size = CheckedElementBytes(
            header.countBytes4Buffer2,
            4,
            "countBytes4Buffer2");
        payload.mainBuffer.bytes4 = Slice(raw, offset, size, "KV3 main bytes4");
        offset += size;
    }

    if (header.countBytes8Buffer2 > 0) {
        Align(offset, 8);
        const auto size = CheckedElementBytes(
            header.countBytes8Buffer2,
            8,
            "countBytes8Buffer2");
        payload.mainBuffer.bytes8 = Slice(raw, offset, size, "KV3 main bytes8");
        offset += size;
    }

    const auto typeBytes = CheckedSize(header.countTypes, "countTypes");
    payload.typeStream = Slice(raw, offset, typeBytes, "KV3 type stream");
    offset += typeBytes;

    if (header.countBlocks == 0) {
        if (ReadU32(raw, offset, "KV3 main trailer") != Kv3Trailer) {
            throw std::runtime_error("KV3 main buffer trailer is invalid");
        }
        offset += 4;
    } else {
        const auto blockCount = CheckedSize(header.countBlocks, "countBlocks");
        const auto blobLengthBytes = blockCount * 4U;
        RequireRange(raw, offset, blobLengthBytes, "KV3 binary blob lengths");
        payload.binaryBlobLengths.reserve(blockCount);
        for (std::size_t index = 0; index < blobLengthBytes; index += 4) {
            payload.binaryBlobLengths.push_back(
                ReadI32(raw, offset + index, "KV3 binary blob length"));
        }
        offset += blobLengthBytes;

        if (ReadU32(raw, offset, "KV3 blob metadata trailer") != Kv3Trailer) {
            throw std::runtime_error("KV3 binary blob metadata trailer is invalid");
        }
        offset += 4;
        compressedBlobSizes = Slice(
            raw,
            offset,
            raw.size() - offset,
            "KV3 compressed blob size table");
        offset = raw.size();
    }

    if (offset != raw.size()) {
        throw std::runtime_error("KV3 main buffer has unexpected trailing bytes");
    }
}

void DecodeBinaryBlobs(
    RawCursor& cursor,
    const BinaryKv3Header& header,
    const std::vector<std::uint8_t>& compressedBlobSizes,
    Kv3Payload& payload,
    const Kv3DecodeLimits& limits) {
    if (header.countBlocks <= 0) {
        return;
    }

    const auto uncompressedSize = CheckedSize(
        header.sizeBinaryBlobsBytes,
        "sizeBinaryBlobsBytes");

    if (header.compression == Kv3Compression::None) {
        payload.binaryBlobs = cursor.ReadBytes(uncompressedSize);
    } else if (header.compression == Kv3Compression::Zstd) {
        const auto compressedTotal = CheckedSize(
            header.sizeCompressedTotal,
            "sizeCompressedTotal");
        const auto compressedBuffer1 = CheckedSize(
            header.sizeCompressedBuffer1,
            "sizeCompressedBuffer1");
        const auto compressedBuffer2 = CheckedSize(
            header.sizeCompressedBuffer2,
            "sizeCompressedBuffer2");
        if (compressedBuffer1 > compressedTotal
            || compressedBuffer2 > compressedTotal - compressedBuffer1) {
            throw std::runtime_error("KV3 compressed binary blob size underflows");
        }
        const auto compressedSize = compressedTotal - compressedBuffer1 - compressedBuffer2;
        const auto compressed = cursor.ReadBytes(compressedSize);
        payload.binaryBlobs = compression::ZstdDecoder{}.Decode(
            compressed.data(),
            compressed.size(),
            uncompressedSize,
            limits.decompression);
    } else if (header.compression == Kv3Compression::Lz4) {
        if (header.compressionFrameSize == 0) {
            throw std::runtime_error("KV3 LZ4 blob stream has a zero frame size");
        }
        if (compressedBlobSizes.size() % 2U != 0U) {
            throw std::runtime_error("KV3 LZ4 blob size table has an odd byte count");
        }

        std::vector<std::vector<std::uint8_t>> blocks;
        std::vector<std::size_t> decodedSizes;
        std::size_t decodedTotal = 0;
        for (std::size_t offset = 0; offset < compressedBlobSizes.size(); offset += 2) {
            const auto compressedSize = static_cast<std::size_t>(
                ReadU16(compressedBlobSizes, offset, "KV3 LZ4 blob block size"));
            blocks.push_back(cursor.ReadBytes(compressedSize));
            const auto remaining = uncompressedSize - decodedTotal;
            const auto decodedSize = std::min<std::size_t>(
                header.compressionFrameSize,
                remaining);
            decodedSizes.push_back(decodedSize);
            decodedTotal += decodedSize;
        }
        if (decodedTotal != uncompressedSize) {
            throw std::runtime_error("KV3 LZ4 blob frames do not cover the expected size");
        }
        payload.binaryBlobs = compression::Lz4Decoder{}.DecodeChainedBlocks(
            blocks,
            decodedSizes,
            uncompressedSize,
            limits.decompression);
    } else {
        throw std::runtime_error("unsupported KV3 binary blob compression");
    }

    if (cursor.ReadU32() != Kv3Trailer) {
        throw std::runtime_error("KV3 DATA trailer is invalid");
    }
}

} // namespace

Kv3Payload Kv3PayloadDecoder::Decode(
    const std::vector<std::uint8_t>& resourceBytes,
    const std::size_t dataOffset,
    const std::size_t dataSize,
    const BinaryKv3Header& header,
    const Kv3DecodeLimits& limits) const {
    if (header.version != 5 || !header.complete) {
        throw std::runtime_error("only complete Binary KV3 version 5 headers are decoded");
    }
    if (header.compressionDictionaryId != 0) {
        throw std::runtime_error("KV3 compression dictionaries are not supported");
    }
    if (header.compression == Kv3Compression::Lz4
        && header.compressionFrameSize != 16384U) {
        throw std::runtime_error("KV3 LZ4 frame size is not 16384 bytes");
    }
    if (header.compression == Kv3Compression::Zstd
        && header.compressionFrameSize != 0U) {
        throw std::runtime_error("KV3 Zstandard frame size is non-zero");
    }
    if (header.headerSize > dataSize) {
        throw std::runtime_error("KV3 header exceeds the DATA block");
    }

    RawCursor cursor(
        resourceBytes,
        dataOffset + header.headerSize,
        dataSize - header.headerSize);

    Kv3Payload payload;
    payload.header = header;

    const auto rawBuffer1 = DecodeBuffer(
        cursor,
        header.compression,
        CheckedSize(header.sizeCompressedBuffer1, "sizeCompressedBuffer1"),
        CheckedSize(header.sizeUncompressedBuffer1, "sizeUncompressedBuffer1"),
        limits);
    const auto rawBuffer2 = DecodeBuffer(
        cursor,
        header.compression,
        CheckedSize(header.sizeCompressedBuffer2, "sizeCompressedBuffer2"),
        CheckedSize(header.sizeUncompressedBuffer2, "sizeUncompressedBuffer2"),
        limits);

    PartitionAuxiliaryBuffer(rawBuffer1, header, payload, limits);

    std::vector<std::uint8_t> compressedBlobSizes;
    PartitionMainBuffer(rawBuffer2, header, payload, compressedBlobSizes);
    DecodeBinaryBlobs(cursor, header, compressedBlobSizes, payload, limits);

    if (cursor.Remaining() != 0) {
        throw std::runtime_error("KV3 DATA block has unread compressed bytes");
    }

    return payload;
}

} // namespace vmsourceconv::serialization::kv3
