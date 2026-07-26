#include "cli/CommandLine.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace vmsourceconv::cli {
namespace {

bool IsHelp(const std::string_view value) {
    return value == "--help" || value == "-h" || value == "/?";
}

bool RequireValue(
    const int argc,
    const int index,
    ParsedCommandLine& result,
    const char* option) {
    if (index + 1 < argc) {
        return true;
    }
    result.error = std::string(option) + " requires a value";
    return false;
}

bool ParseBoundedU64(
    const std::string_view value,
    const std::uint64_t minimum,
    const std::uint64_t maximum,
    std::uint64_t& output) {
    std::uint64_t parsed = 0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end
        || parsed < minimum || parsed > maximum) {
        return false;
    }
    output = parsed;
    return true;
}

bool ParseBoundedSize(
    const std::string_view value,
    const std::uint64_t minimum,
    const std::uint64_t maximum,
    std::size_t& output) {
    std::uint64_t parsed = 0;
    if (!ParseBoundedU64(value, minimum, maximum, parsed)) {
        return false;
    }
    output = static_cast<std::size_t>(parsed);
    return true;
}

bool EndsWithIgnoreCase(
    const std::string_view value,
    const std::string_view suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(
        suffix.rbegin(), suffix.rend(), value.rbegin(),
        [](const unsigned char left, const unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        });
}

enum class GlobalOptionResult { NotHandled, Handled, Error };

GlobalOptionResult ParseGlobalOption(
    const int argc,
    char** argv,
    int& index,
    ParsedCommandLine& result) {
    const std::string_view argument{argv[index]};
    if (argument == "--log") {
        if (!RequireValue(argc, index, result, "--log")) {
            return GlobalOptionResult::Error;
        }
        result.logOptions.path = argv[++index];
        return GlobalOptionResult::Handled;
    }
    constexpr std::string_view logPrefix = "--log=";
    if (argument.size() > logPrefix.size()
        && argument.substr(0, logPrefix.size()) == logPrefix) {
        result.logOptions.path = std::string(argument.substr(logPrefix.size()));
        return GlobalOptionResult::Handled;
    }
    if (argument == "--log-mode") {
        if (!RequireValue(argc, index, result, "--log-mode")) {
            return GlobalOptionResult::Error;
        }
        const std::string_view value{argv[++index]};
        if (value == "append") {
            result.logOptions.mode = core::LogMode::Append;
        } else if (value == "overwrite") {
            result.logOptions.mode = core::LogMode::Overwrite;
        } else {
            result.error = "--log-mode must be append or overwrite";
            return GlobalOptionResult::Error;
        }
        return GlobalOptionResult::Handled;
    }
    if (argument == "--quiet") {
        result.logOptions.verbosity = core::Verbosity::Quiet;
        return GlobalOptionResult::Handled;
    }
    if (argument == "--verbose") {
        result.logOptions.verbosity = core::Verbosity::Verbose;
        return GlobalOptionResult::Handled;
    }
    if (argument == "--debug") {
        result.logOptions.verbosity = core::Verbosity::Debug;
        return GlobalOptionResult::Handled;
    }
    if (argument == "--no-log-timestamps") {
        result.logOptions.timestamps = false;
        return GlobalOptionResult::Handled;
    }
    if (argument.size() > 2U
        && argument.substr(0, 2) == "--"
        && EndsWithIgnoreCase(argument, ".txt")
        && index == argc - 1
        && result.logOptions.path.empty()) {
        result.logOptions.path = std::string(argument.substr(2));
        return GlobalOptionResult::Handled;
    }
    return GlobalOptionResult::NotHandled;
}

bool ParseByteLimit(
    const int argc,
    char** argv,
    int& index,
    ParsedCommandLine& result,
    const char* option,
    std::uint64_t& output) {
    if (!RequireValue(argc, index, result, option)) {
        return false;
    }
    constexpr std::uint64_t Maximum = 16ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    const std::string_view value{argv[++index]};
    if (!ParseBoundedU64(value, 1U, Maximum, output)) {
        result.error = std::string(option)
            + " must be an integer from 1 to 17592186044416 bytes";
        return false;
    }
    return true;
}

} // namespace
