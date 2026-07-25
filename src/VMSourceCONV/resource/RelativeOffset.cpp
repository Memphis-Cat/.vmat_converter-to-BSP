#include "resource/RelativeOffset.h"

#include "core/ParseError.h"

#include <limits>

namespace vmsourceconv::resource {

std::size_t CheckedAdd(const std::size_t left, const std::size_t right) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        throw core::ParseError("offset addition overflow", left);
    }
    return left + right;
}

std::size_t CheckedMultiply(const std::size_t left, const std::size_t right) {
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left) {
        throw core::ParseError("size multiplication overflow", left);
    }
    return left * right;
}

std::size_t ResolveRelativeOffset(const std::size_t fieldOffset, const std::uint64_t relativeOffset) {
    if (relativeOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw core::ParseError("relative offset is too large", fieldOffset);
    }
    return CheckedAdd(fieldOffset, static_cast<std::size_t>(relativeOffset));
}

std::size_t ResolveSignedRelativeOffset(const std::size_t fieldOffset, const std::int64_t relativeOffset) {
    if (relativeOffset >= 0) {
        return ResolveRelativeOffset(fieldOffset, static_cast<std::uint64_t>(relativeOffset));
    }

    const auto magnitude = static_cast<std::uint64_t>(-(relativeOffset + 1)) + 1U;
    if (magnitude > fieldOffset) {
        throw core::ParseError("negative relative offset points before the file", fieldOffset);
    }
    return fieldOffset - static_cast<std::size_t>(magnitude);
}

} // namespace vmsourceconv::resource
