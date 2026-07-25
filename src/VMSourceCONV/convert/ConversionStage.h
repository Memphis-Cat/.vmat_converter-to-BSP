#pragma once

namespace vmsourceconv::convert {

class ConversionStage final {
public:
    [[nodiscard]] static const char* Name() noexcept;
    [[nodiscard]] static bool IsImplemented() noexcept;
};

} // namespace vmsourceconv::convert
