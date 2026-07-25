#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::io {

class BinaryReader final {
public:
    explicit BinaryReader(const std::vector<std::uint8_t>& bytes);

    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t Position() const noexcept;
    [[nodiscard]] std::size_t Remaining() const noexcept;

    void Seek(std::size_t position);
    void Require(std::size_t byteCount) const;

    std::uint8_t ReadU8();
    std::uint16_t ReadU16();
    std::uint32_t ReadU32();
    std::uint64_t ReadU64();
    std::int64_t ReadI64();
    std::string ReadCString(std::size_t maximumEnd);

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
};

} // namespace vmsourceconv::io
