#pragma once

#include <cstddef>
#include <cstdint>

namespace vmsourceconv::resource {

std::size_t ResolveRelativeOffset(std::size_t fieldOffset, std::uint64_t relativeOffset);
std::size_t ResolveSignedRelativeOffset(std::size_t fieldOffset, std::int64_t relativeOffset);
std::size_t CheckedAdd(std::size_t left, std::size_t right);
std::size_t CheckedMultiply(std::size_t left, std::size_t right);

} // namespace vmsourceconv::resource
