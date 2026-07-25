#pragma once

namespace vmsourceconv::decision {

class DecisionStage final {
public:
    [[nodiscard]] static const char* Name() noexcept;
    [[nodiscard]] static bool IsImplemented() noexcept;
};

} // namespace vmsourceconv::decision
