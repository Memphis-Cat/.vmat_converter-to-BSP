#include "core/LogOptions.h"

namespace vmsourceconv::core {

const char* ToString(const LogMode mode) noexcept {
    switch (mode) {
        case LogMode::Overwrite: return "overwrite";
        case LogMode::Append: return "append";
    }
    return "unknown";
}

const char* ToString(const Verbosity verbosity) noexcept {
    switch (verbosity) {
        case Verbosity::Quiet: return "quiet";
        case Verbosity::Normal: return "normal";
        case Verbosity::Verbose: return "verbose";
        case Verbosity::Debug: return "debug";
    }
    return "unknown";
}

} // namespace vmsourceconv::core
