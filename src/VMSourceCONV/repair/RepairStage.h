#pragma once

namespace vmsourceconv::repair {

class RepairStage final {
public:
    [[nodiscard]] static const char* Name() noexcept;
    [[nodiscard]] static bool IsImplemented() noexcept;
};

} // namespace vmsourceconv::repair
