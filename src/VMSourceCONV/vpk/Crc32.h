#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vmsourceconv::vpk {

[[nodiscard]] std::uint32_t ComputeCrc32(
    const std::uint8_t* data,
    std::size_t size) noexcept;

[[nodiscard]] inline std::uint32_t ComputeCrc32(
    const std::vector<std::uint8_t>& data) noexcept {
    return ComputeCrc32(data.data(), data.size());
}

} // namespace vmsourceconv::vpk
