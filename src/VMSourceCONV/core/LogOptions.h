#pragma once

#include <filesystem>

namespace vmsourceconv::core {

enum class LogMode {
    Overwrite,
    Append,
};

enum class Verbosity {
    Quiet,
    Normal,
    Verbose,
    Debug,
};

struct LogOptions {
    std::filesystem::path path;
    LogMode mode = LogMode::Overwrite;
    Verbosity verbosity = Verbosity::Normal;
    bool timestamps = true;
};

[[nodiscard]] const char* ToString(LogMode mode) noexcept;
[[nodiscard]] const char* ToString(Verbosity verbosity) noexcept;

} // namespace vmsourceconv::core
