#pragma once

#include <cstddef>
#include <cstdint>

namespace vmsourceconv::resource {

struct ResourceHeader {
    std::uint32_t declaredFileSize = 0;
    std::uint16_t headerVersion = 0;
    std::uint16_t resourceVersion = 0;
    std::uint32_t blockDirectoryRelativeOffset = 0;
    std::uint32_t blockCount = 0;
    std::size_t blockDirectoryOffset = 0;
};

constexpr std::uint16_t KnownResourceHeaderVersion() noexcept {
    return 12;
}

} // namespace vmsourceconv::resource
