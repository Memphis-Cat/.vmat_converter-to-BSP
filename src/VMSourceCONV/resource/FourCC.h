#pragma once

#include <cstdint>
#include <string>

namespace vmsourceconv::resource {

class FourCC final {
public:
    FourCC() = default;
    explicit FourCC(std::uint32_t value);

    [[nodiscard]] std::uint32_t Value() const noexcept;
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] bool Is(const char (&text)[5]) const noexcept;

private:
    std::uint32_t value_ = 0;
};

} // namespace vmsourceconv::resource
