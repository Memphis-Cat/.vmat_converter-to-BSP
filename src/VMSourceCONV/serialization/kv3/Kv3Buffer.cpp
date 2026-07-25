#include "serialization/kv3/Kv3Buffer.h"

#include <cstring>
#include <stdexcept>

namespace vmsourceconv::serialization::kv3 {

std::uint8_t Kv3Buffer::ReadU8() {
    Require(bytes1, position1_, 1);
    return bytes1[position1_++];
}

std::int16_t Kv3Buffer::ReadI16() {
    return static_cast<std::int16_t>(ReadU16());
}

std::uint16_t Kv3Buffer::ReadU16() {
    return ReadU16At(position2_);
}

std::int32_t Kv3Buffer::ReadI32() {
    return static_cast<std::int32_t>(ReadU32());
}

std::uint32_t Kv3Buffer::ReadU32() {
    return ReadU32At(position4_);
}

float Kv3Buffer::ReadFloat() {
    const auto bits = ReadU32();
    float value = 0.0F;
    static_assert(sizeof(value) == sizeof(bits), "unexpected float size");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::int64_t Kv3Buffer::ReadI64() {
    return static_cast<std::int64_t>(ReadU64());
}

std::uint64_t Kv3Buffer::ReadU64() {
    return ReadU64At(position8_);
}

double Kv3Buffer::ReadDouble() {
    const auto bits = ReadU64();
    double value = 0.0;
    static_assert(sizeof(value) == sizeof(bits), "unexpected double size");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

std::string Kv3Buffer::ReadCString(const std::size_t maximumBytes) {
    const auto start = position1_;
    while (position1_ < bytes1.size() && bytes1[position1_] != 0U) {
        if (position1_ - start >= maximumBytes) {
            throw std::runtime_error("KV3 string exceeds the configured size limit");
        }
        ++position1_;
    }
    if (position1_ >= bytes1.size()) {
        throw std::runtime_error("KV3 string table contains an unterminated string");
    }

    const std::string value(
        reinterpret_cast<const char*>(bytes1.data() + start),
        position1_ - start);
    ++position1_;
    return value;
}

std::size_t Kv3Buffer::Remaining1() const noexcept { return bytes1.size() - position1_; }
std::size_t Kv3Buffer::Remaining2() const noexcept { return bytes2.size() - position2_; }
std::size_t Kv3Buffer::Remaining4() const noexcept { return bytes4.size() - position4_; }
std::size_t Kv3Buffer::Remaining8() const noexcept { return bytes8.size() - position8_; }

std::uint16_t Kv3Buffer::ReadU16At(std::size_t& position) {
    Require(bytes2, position, 2);
    const auto value = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(bytes2[position])
        | static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(bytes2[position + 1]) << 8U));
    position += 2;
    return value;
}

std::uint32_t Kv3Buffer::ReadU32At(std::size_t& position) {
    Require(bytes4, position, 4);
    const auto value = static_cast<std::uint32_t>(bytes4[position])
        | (static_cast<std::uint32_t>(bytes4[position + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes4[position + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes4[position + 3]) << 24U);
    position += 4;
    return value;
}

std::uint64_t Kv3Buffer::ReadU64At(std::size_t& position) {
    Require(bytes8, position, 8);
    std::uint64_t value = 0;
    for (unsigned int index = 0; index < 8U; ++index) {
        value |= static_cast<std::uint64_t>(bytes8[position + index]) << (index * 8U);
    }
    position += 8;
    return value;
}

void Kv3Buffer::Require(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t position,
    const std::size_t size) const {
    if (position > bytes.size() || size > bytes.size() - position) {
        throw std::runtime_error("KV3 scalar buffer is truncated");
    }
}

} // namespace vmsourceconv::serialization::kv3
