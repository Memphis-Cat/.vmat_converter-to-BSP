#pragma once

namespace vmsourceconv::fix {

class FixStage final {
public:
    [[nodiscard]] static const char* Name() noexcept;
    [[nodiscard]] static bool IsImplemented() noexcept;
};

} // namespace vmsourceconv::fix
