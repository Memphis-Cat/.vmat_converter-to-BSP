#include "vpk/VpkVerification.h"

#include <algorithm>

namespace vmsourceconv::vpk {

bool VpkVerificationReport::Success() const noexcept {
    return ErrorCount() == 0U;
}

std::size_t VpkVerificationReport::WarningCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        issues.begin(), issues.end(),
        [](const VpkVerificationIssue& issue) {
            return issue.severity == VpkVerificationSeverity::Warning;
        }));
}

std::size_t VpkVerificationReport::ErrorCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        issues.begin(), issues.end(),
        [](const VpkVerificationIssue& issue) {
            return issue.severity == VpkVerificationSeverity::Error;
        }));
}

const char* ToString(const VpkVerificationSeverity severity) noexcept {
    switch (severity) {
        case VpkVerificationSeverity::Info: return "info";
        case VpkVerificationSeverity::Warning: return "warning";
        case VpkVerificationSeverity::Error: return "error";
    }
    return "unknown";
}

} // namespace vmsourceconv::vpk
