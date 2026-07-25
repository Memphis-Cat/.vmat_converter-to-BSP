#include "core/ParseError.h"

#include <utility>

namespace vmsourceconv::core {

ParseError::ParseError(std::string message, const std::size_t offset)
    : std::runtime_error(std::move(message)), offset_(offset) {}

std::size_t ParseError::Offset() const noexcept {
    return offset_;
}

} // namespace vmsourceconv::core
