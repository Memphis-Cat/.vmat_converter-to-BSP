#include "compression/CompressionError.h"

namespace vmsourceconv::compression {

CompressionError::CompressionError(const std::string& message)
    : std::runtime_error(message) {}

} // namespace vmsourceconv::compression
