#include "core/Diagnostic.h"

namespace vmsourceconv::core {

const char* ToString(const DiagnosticSeverity severity) noexcept {
    switch (severity) {
        case DiagnosticSeverity::Info:
            return "info";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Error:
            return "error";
    }

    return "unknown";
}

} // namespace vmsourceconv::core
