#pragma once

namespace vmsourceconv::core {

enum class ExitCode : int {
    Success = 0,
    UsageError = 2,
    IoError = 3,
    ParseError = 4,
    OutputError = 5,
    InternalError = 10,
};

} // namespace vmsourceconv::core
