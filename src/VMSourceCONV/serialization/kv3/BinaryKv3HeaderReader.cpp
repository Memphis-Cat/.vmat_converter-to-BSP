#include "serialization/kv3/BinaryKv3HeaderReader.h"

#include "serialization/kv3/BinaryKv3Magic.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace vmsourceconv::serialization::kv3 {
namespace {

class Cursor final {
public:
    Cursor(
        const std::vector<std::uint8_t>& bytes,
        const std::size_t offset,
        const std::size_t size)
        : bytes_(bytes), begin_(offset), position_(offset) {
        if (offset > bytes.size() || size > bytes.size() - offset) {
            throw std::runtime_error("Binary KV3 range is outside the resource");
        }
        end_ = offset + size;
    }

    [[nodiscard]] std::size_t Consumed() const noexcept {
        return position_ - begin_;
    }

    [[nodiscard]] std::size_t Remaining() const noexcept {
        return end_ - position_;
    }

    std::uint16_t ReadU16() {
        Require(2);
        const auto low = static_cast<std::uint16_t>(bytes_[position_]);
        const auto high = static_cast<std::uint16_t>(bytes_[position_ + 1]);
        position_ += 2;
        return static_cast<std::uint16_t>(
            low | static_cast<std::uint16_t>(high << 8U));
    }

    std::uint32_t ReadU32() {
        Require(4);
        const auto value = static_cast<std::uint32_t>(bytes_[position_])
            | (static_cast<std::uint32_t>(bytes_[position_ + 1]) << 8U)
            | (static_cast<std::uint32_t>(bytes_[position_ + 2]) << 16U)
            | (static_cast<std::uint32_t>(bytes_[position_ + 3]) << 24U);
        position_ += 4;
        return value;
    }

    std::int32_t ReadI32() {
        return static_cast<std::int32_t>(ReadU32());
    }

    std::array<std::uint8_t, 16> ReadGuidBytes() {
        Require(16);
        std::array<std::uint8_t, 16> value{};
        std::copy_n(
            bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
            16,
            value.begin());
        position_ += 16;
        return value;
    }

private:
    void Require(const std::size_t count) const {
        if (count > end_ - position_) {
            throw std::runtime_error("Binary KV3 header is truncated");
        }
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t begin_ = 0;
    std::size_t position_ = 0;
    std::size_t end_ = 0;
};

void WarnIfNegative(
    const std::int32_t value,
    const char* name,
    std::vector<std::string>& warnings) {
    if (value < 0) {
        warnings.emplace_back(std::string(name) + " is negative");
    }
}

} // namespace

BinaryKv3Header BinaryKv3HeaderReader::Read(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::size_t size) const {
    Cursor cursor(bytes, offset, size);
    BinaryKv3Header header;

    header.magic = cursor.ReadU32();
    header.version = BinaryKv3Version(header.magic);
    if (header.version < 0) {
        throw std::runtime_error(
            "DATA block does not contain a supported Binary KV3 magic");
    }

    if (header.version == 0) {
        header.headerSize = cursor.Consumed();
        header.warnings.emplace_back(
            "legacy Binary KV3 version 0 payload metadata is not decoded yet");
        return header;
    }

    header.formatId = cursor.ReadGuidBytes();
    header.compressionMethod = cursor.ReadU32();
    header.compression = Kv3CompressionFromMethod(header.compressionMethod);

    if (header.version == 1) {
        header.countBytes1 = cursor.ReadI32();
        header.countBytes4 = cursor.ReadI32();
        header.countBytes8 = cursor.ReadI32();
        header.sizeUncompressedTotal = cursor.ReadI32();
        if (cursor.Remaining() > static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())) {
            header.warnings.emplace_back(
                "compressed payload is larger than an int32");
            header.sizeCompressedTotal =
                std::numeric_limits<std::int32_t>::max();
        } else {
            header.sizeCompressedTotal =
                static_cast<std::int32_t>(cursor.Remaining());
        }
    } else {
        header.compressionDictionaryId = cursor.ReadU16();
        header.compressionFrameSize = cursor.ReadU16();
        header.countBytes1 = cursor.ReadI32();
        header.countBytes4 = cursor.ReadI32();
        header.countBytes8 = cursor.ReadI32();
        header.countTypes = cursor.ReadI32();
        header.countObjects = cursor.ReadU16();
        header.countArrays = cursor.ReadU16();
        header.sizeUncompressedTotal = cursor.ReadI32();
        header.sizeCompressedTotal = cursor.ReadI32();
        header.countBlocks = cursor.ReadI32();
        header.sizeBinaryBlobsBytes = cursor.ReadI32();
    }

    if (header.version >= 4) {
        header.countBytes2 = cursor.ReadI32();
        header.sizeBlockCompressedSizesBytes = cursor.ReadI32();
    }

    if (header.version >= 5) {
        header.sizeUncompressedBuffer1 = cursor.ReadI32();
        header.sizeCompressedBuffer1 = cursor.ReadI32();
        header.sizeUncompressedBuffer2 = cursor.ReadI32();
        header.sizeCompressedBuffer2 = cursor.ReadI32();
        header.countBytes1Buffer2 = cursor.ReadI32();
        header.countBytes2Buffer2 = cursor.ReadI32();
        header.countBytes4Buffer2 = cursor.ReadI32();
        header.countBytes8Buffer2 = cursor.ReadI32();
        (void)cursor.ReadI32();
        header.countObjectsBuffer2 = cursor.ReadI32();
        header.countArraysBuffer2 = cursor.ReadI32();
        (void)cursor.ReadI32();
    }

    header.headerSize = cursor.Consumed();
    header.complete = true;

    WarnIfNegative(header.countBytes1, "countBytes1", header.warnings);
    WarnIfNegative(header.countBytes2, "countBytes2", header.warnings);
    WarnIfNegative(header.countBytes4, "countBytes4", header.warnings);
    WarnIfNegative(header.countBytes8, "countBytes8", header.warnings);
    WarnIfNegative(header.countTypes, "countTypes", header.warnings);
    WarnIfNegative(
        header.sizeUncompressedTotal,
        "sizeUncompressedTotal",
        header.warnings);
    WarnIfNegative(
        header.sizeCompressedTotal,
        "sizeCompressedTotal",
        header.warnings);
    WarnIfNegative(header.countBlocks, "countBlocks", header.warnings);
    WarnIfNegative(
        header.sizeBinaryBlobsBytes,
        "sizeBinaryBlobsBytes",
        header.warnings);

    if (header.compression == Kv3Compression::Unknown) {
        header.warnings.emplace_back("unknown Binary KV3 compression method");
    }

    if (header.compressionDictionaryId != 0) {
        header.warnings.emplace_back("non-zero compression dictionary id");
    }

    if (header.version >= 5
        && header.sizeUncompressedBuffer1 >= 0
        && header.sizeUncompressedBuffer2 >= 0
        && header.sizeUncompressedTotal >= 0
        && static_cast<std::int64_t>(header.sizeUncompressedBuffer1)
            + static_cast<std::int64_t>(header.sizeUncompressedBuffer2)
            != static_cast<std::int64_t>(header.sizeUncompressedTotal)) {
        header.warnings.emplace_back(
            "version 5 buffer sizes do not add up to the total uncompressed size");
    }

    return header;
}

} // namespace vmsourceconv::serialization::kv3
