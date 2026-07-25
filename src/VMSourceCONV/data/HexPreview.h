#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::data {

[[nodiscard]] std::string MakeHexPreview(
    const std::vector<std::uint8_t>& bytes,
    std::size_t offset,
    std::size_t size,
    std::size_t maximumBytes = 32);

} // namespace vmsourceconv::data
