#pragma once

#include "resource/FourCC.h"

#include <cstddef>
#include <cstdint>

namespace vmsourceconv::resource {

struct ResourceBlock {
    std::size_t index = 0;
    FourCC type;
    std::size_t directoryEntryOffset = 0;
    std::uint32_t relativeOffset = 0;
    std::size_t offset = 0;
    std::uint32_t size = 0;

    [[nodiscard]] std::size_t EndOffset() const;
};

} // namespace vmsourceconv::resource
