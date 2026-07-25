#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace vmsourceconv::core {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string code;
    std::string message;
    std::optional<std::size_t> offset;
};

const char* ToString(DiagnosticSeverity severity) noexcept;

} // namespace vmsourceconv::core
