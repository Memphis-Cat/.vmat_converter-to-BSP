#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

class Kv3Buffer final {
public:
    std::vector<std::uint8_t> bytes1;
    std::vector<std::uint8_t> bytes2;
    std::vector<std::uint8_t> bytes4;
    std::vector<std::uint8_t> bytes8;

    [[nodiscard]] std::uint8_t ReadU8();
    [[nodiscard]] std::int16_t ReadI16();
    [[nodiscard]] std::uint16_t ReadU16();
    [[nodiscard]] std::int32_t ReadI32();
    [[nodiscard]] std::uint32_t ReadU32();
    [[nodiscard]] float ReadFloat();
    [[nodiscard]] std::int64_t ReadI64();
    [[nodiscard]] std::uint64_t ReadU64();
    [[nodiscard]] double ReadDouble();
    [[nodiscard]] std::string ReadCString(std::size_t maximumBytes);

    [[nodiscard]] std::size_t Remaining1() const noexcept;
    [[nodiscard]] std::size_t Remaining2() const noexcept;
    [[nodiscard]] std::size_t Remaining4() const noexcept;
    [[nodiscard]] std::size_t Remaining8() const noexcept;

private:
    [[nodiscard]] std::uint16_t ReadU16At(std::size_t& position);
    [[nodiscard]] std::uint32_t ReadU32At(std::size_t& position);
    [[nodiscard]] std::uint64_t ReadU64At(std::size_t& position);
    void Require(const std::vector<std::uint8_t>& bytes, std::size_t position, std::size_t size) const;

    std::size_t position1_ = 0;
    std::size_t position2_ = 0;
    std::size_t position4_ = 0;
    std::size_t position8_ = 0;
};

} // namespace vmsourceconv::serialization::kv3
