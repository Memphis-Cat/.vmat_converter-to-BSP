#include "inspect/InspectionReport.h"

#include <algorithm>

namespace vmsourceconv::inspect {

std::size_t InspectionReport::WarningCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        diagnostics.begin(), diagnostics.end(), [](const auto& diagnostic) {
            return diagnostic.severity == core::DiagnosticSeverity::Warning;
        }));
}

std::size_t InspectionReport::ErrorCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        diagnostics.begin(), diagnostics.end(), [](const auto& diagnostic) {
            return diagnostic.severity == core::DiagnosticSeverity::Error;
        }));
}

} // namespace vmsourceconv::inspect
