#pragma once

#include <stdexcept>
#include <string>

namespace vmsourceconv::compression {

class CompressionError final : public std::runtime_error {
public:
    explicit CompressionError(const std::string& message);
};

} // namespace vmsourceconv::compression
