#pragma once

namespace vmsourceconv::analyzing {

class AnalysisStage final {
public:
    [[nodiscard]] static const char* Name() noexcept;
    [[nodiscard]] static bool IsImplemented() noexcept;
};

} // namespace vmsourceconv::analyzing
