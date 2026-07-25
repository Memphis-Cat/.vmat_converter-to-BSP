#include "io/BinaryReader.h"

#include "core/ParseError.h"

#include <limits>

namespace vmsourceconv::io {
namespace {

template <typename T>
T ReadLittleEndian(const std::vector<std::uint8_t>& bytes, const std::size_t position) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < sizeof(T); ++index) {
        value |= static_cast<std::uint64_t>(bytes[position + index]) << (index * 8U);
    }
    return static_cast<T>(value);
}

} // namespace

BinaryReader::BinaryReader(const std::vector<std::uint8_t>& bytes) : bytes_(bytes) {}

std::size_t BinaryReader::Size() const noexcept {
    return bytes_.size();
}

std::size_t BinaryReader::Position() const noexcept {
    return position_;
}

std::size_t BinaryReader::Remaining() const noexcept {
    return position_ <= bytes_.size() ? bytes_.size() - position_ : 0;
}

void BinaryReader::Seek(const std::size_t position) {
    if (position > bytes_.size()) {
        throw core::ParseError("seek is outside the input file", position);
    }
    position_ = position;
}

void BinaryReader::Require(const std::size_t byteCount) const {
    if (byteCount > Remaining()) {
        throw core::ParseError("unexpected end of file", position_);
    }
}

std::uint8_t BinaryReader::ReadU8() {
    Require(1);
    return bytes_[position_++];
}

std::uint16_t BinaryReader::ReadU16() {
    Require(sizeof(std::uint16_t));
    const auto value = ReadLittleEndian<std::uint16_t>(bytes_, position_);
    position_ += sizeof(std::uint16_t);
    return value;
}

std::uint32_t BinaryReader::ReadU32() {
    Require(sizeof(std::uint32_t));
    const auto value = ReadLittleEndian<std::uint32_t>(bytes_, position_);
    position_ += sizeof(std::uint32_t);
    return value;
}

std::uint64_t BinaryReader::ReadU64() {
    Require(sizeof(std::uint64_t));
    const auto value = ReadLittleEndian<std::uint64_t>(bytes_, position_);
    position_ += sizeof(std::uint64_t);
    return value;
}

std::int64_t BinaryReader::ReadI64() {
    return static_cast<std::int64_t>(ReadU64());
}

std::string BinaryReader::ReadCString(const std::size_t maximumEnd) {
    if (maximumEnd > bytes_.size() || position_ > maximumEnd) {
        throw core::ParseError("invalid C string boundary", position_);
    }

    const auto begin = position_;
    while (position_ < maximumEnd && bytes_[position_] != 0) {
        ++position_;
    }

    if (position_ >= maximumEnd) {
        throw core::ParseError("unterminated UTF-8 string", begin);
    }

    const std::string value(
        reinterpret_cast<const char*>(bytes_.data() + begin),
        position_ - begin);
    ++position_;
    return value;
}

} // namespace vmsourceconv::io
