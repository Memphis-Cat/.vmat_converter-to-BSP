#include "cli/CommandLine.h"

#include <string_view>

namespace vmsourceconv::cli {
namespace {

bool IsHelp(const std::string_view value) {
    return value == "--help" || value == "-h" || value == "/?";
}

bool RequireValue(const int argc, const int index, ParsedCommandLine& result, const char* option) {
    if (index + 1 < argc) {
        return true;
    }

    result.error = std::string(option) + " requires a value";
    return false;
}

} // namespace

ParsedCommandLine CommandLine::Parse(const int argc, char** argv) {
    ParsedCommandLine result;

    if (argc <= 1) {
        result.showHelp = true;
        return result;
    }

    const std::string_view first{argv[1]};
    if (IsHelp(first)) {
        result.showHelp = true;
        return result;
    }

    if (first == "--version" || first == "-v") {
        result.showVersion = true;
        return result;
    }

    if (first != "inspect") {
        result.error = "unknown command: " + std::string(first);
        return result;
    }

    result.command = CommandKind::Inspect;

    for (int index = 2; index < argc; ++index) {
        const std::string_view argument{argv[index]};

        if (IsHelp(argument)) {
            result.showHelp = true;
            return result;
        }

        if (argument == "--json") {
            if (!RequireValue(argc, index, result, "--json")) {
                return result;
            }
            result.inspectOptions.jsonOutput = argv[++index];
            continue;
        }

        if (argument == "--dump-blocks") {
            if (!RequireValue(argc, index, result, "--dump-blocks")) {
                return result;
            }
            result.inspectOptions.dumpDirectory = argv[++index];
            continue;
        }

        if (argument == "--no-rerl") {
            result.inspectOptions.inspectExternalReferences = false;
            continue;
        }

        if (argument == "--strict") {
            result.inspectOptions.strict = true;
            continue;
        }

        if (!argument.empty() && argument.front() == '-') {
            result.error = "unknown inspect option: " + std::string(argument);
            return result;
        }

        if (!result.inspectOptions.input.empty()) {
            result.error = "inspect accepts exactly one input file";
            return result;
        }

        result.inspectOptions.input = argv[index];
    }

    if (result.inspectOptions.input.empty()) {
        result.error = "inspect requires a .vmap_c or other Source 2 compiled resource";
    }

    return result;
}

} // namespace vmsourceconv::cli
