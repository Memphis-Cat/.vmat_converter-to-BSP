#include "vpk/Crc32.h"

#include <array>

namespace vmsourceconv::vpk {
namespace {

constexpr std::array<std::uint32_t, 256> MakeTable() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t index = 0; index < table.size(); ++index) {
        auto value = index;
        for (int bit = 0; bit < 8; ++bit) {
            value = (value & 1U) != 0U
                ? 0xEDB88320U ^ (value >> 1U)
                : value >> 1U;
        }
        table[index] = value;
    }
    return table;
}

constexpr auto Table = MakeTable();

} // namespace

std::uint32_t ComputeCrc32(
    const std::uint8_t* data,
    const std::size_t size) noexcept {
    auto crc = 0xFFFFFFFFU;
    for (std::size_t index = 0; index < size; ++index) {
        const auto tableIndex = static_cast<std::uint8_t>(
            (crc ^ data[index]) & 0xFFU);
        crc = Table[tableIndex] ^ (crc >> 8U);
    }
    return crc ^ 0xFFFFFFFFU;
}

} // namespace vmsourceconv::vpk
