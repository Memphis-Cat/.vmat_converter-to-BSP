#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace vmsourceconv::core {

class ParseError final : public std::runtime_error {
public:
    ParseError(std::string message, std::size_t offset);

    [[nodiscard]] std::size_t Offset() const noexcept;

private:
    std::size_t offset_;
};

} // namespace vmsourceconv::core
