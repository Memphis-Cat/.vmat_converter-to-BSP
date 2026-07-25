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

bool ParseBoundedSize(
    const std::string_view value,
    const std::uint64_t minimum,
    const std::uint64_t maximum,
    std::size_t& output) {
    std::uint64_t parsed = 0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end
        || parsed < minimum || parsed > maximum) {
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
        suffix.rbegin(),
        suffix.rend(),
        value.rbegin(),
        [](const unsigned char left, const unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        });
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
            if (!RequireValue(argc, index, result, "--json")) return result;
            result.inspectOptions.jsonOutput = argv[++index];
            continue;
        }
        if (argument == "--entities-json") {
            if (!RequireValue(argc, index, result, "--entities-json")) return result;
            result.inspectOptions.entitiesJsonOutput = argv[++index];
            result.inspectOptions.inspectData = true;
            result.inspectOptions.decodeEntityLumps = true;
            continue;
        }
        if (argument == "--kv3-json-dir") {
            if (!RequireValue(argc, index, result, "--kv3-json-dir")) return result;
            result.inspectOptions.kv3JsonDirectory = argv[++index];
            result.inspectOptions.inspectData = true;
            result.inspectOptions.decodeKv3 = true;
            continue;
        }
        if (argument == "--kv3-filter") {
            if (!RequireValue(argc, index, result, "--kv3-filter")) return result;
            result.inspectOptions.kv3Filters.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--scene-json") {
            if (!RequireValue(argc, index, result, "--scene-json")) return result;
            result.inspectOptions.sceneJsonOutput = argv[++index];
            result.inspectOptions.inspectData = true;
            result.inspectOptions.decodeKv3 = true;
            result.inspectOptions.reconstructScene = true;
            continue;
        }
        if (argument == "--dump-blocks") {
            if (!RequireValue(argc, index, result, "--dump-blocks")) return result;
            result.inspectOptions.dumpDirectory = argv[++index];
            continue;
        }
        if (argument == "--resource-root") {
            if (!RequireValue(argc, index, result, "--resource-root")) return result;
            result.inspectOptions.resourceRoots.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--vpk") {
            if (!RequireValue(argc, index, result, "--vpk")) return result;
            result.inspectOptions.vpkPaths.emplace_back(argv[++index]);
            continue;
        }
        if (argument == "--log") {
            if (!RequireValue(argc, index, result, "--log")) return result;
            result.inspectOptions.logOutput = argv[++index];
            continue;
        }

        constexpr std::string_view logPrefix = "--log=";
        if (argument.size() > logPrefix.size()
            && argument.substr(0, logPrefix.size()) == logPrefix) {
            result.inspectOptions.logOutput =
                std::string(argument.substr(logPrefix.size()));
            continue;
        }
        if (argument.size() > 2
            && argument.substr(0, 2) == "--"
            && EndsWithIgnoreCase(argument, ".txt")) {
            result.inspectOptions.logOutput = std::string(argument.substr(2));
            continue;
        }

        if (argument == "--max-depth") {
            if (!RequireValue(argc, index, result, "--max-depth")) return result;
            const std::string_view value{argv[++index]};
            if (!ParseBoundedSize(
                    value, 1, 64, result.inspectOptions.maximumDepth)) {
                result.error = "--max-depth must be an integer from 1 to 64";
                return result;
            }
            continue;
        }
        if (argument == "--max-resources") {
            if (!RequireValue(argc, index, result, "--max-resources")) return result;
            const std::string_view value{argv[++index]};
            if (!ParseBoundedSize(
                    value, 1, 1'000'000,
                    result.inspectOptions.maximumResources)) {
                result.error =
                    "--max-resources must be an integer from 1 to 1000000";
                return result;
            }
            continue;
        }

        if (argument == "--follow-references") {
            result.inspectOptions.followReferences = true;
            continue;
        }
        if (argument == "--include-assets") {
            result.inspectOptions.followReferences = true;
            result.inspectOptions.includeAssets = true;
            continue;
        }
        if (argument == "--inspect-data") {
            result.inspectOptions.inspectData = true;
            continue;
        }
        if (argument == "--decode-kv3") {
            result.inspectOptions.inspectData = true;
            result.inspectOptions.decodeKv3 = true;
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
        result.error =
            "inspect requires a .vmap_c or other Source 2 compiled resource";
        return result;
    }

    if (!result.inspectOptions.kv3Filters.empty()
        && result.inspectOptions.kv3JsonDirectory.empty()) {
        result.error = "--kv3-filter requires --kv3-json-dir";
        return result;
    }

    const auto inputFileName = result.inspectOptions.input.filename().string();
    if (result.inspectOptions.decodeEntityLumps
        && !EndsWithIgnoreCase(inputFileName, ".vents_c")) {
        result.inspectOptions.followReferences = true;
    }
    if (result.inspectOptions.reconstructScene
        && !EndsWithIgnoreCase(inputFileName, ".vwrld_c")
        && !EndsWithIgnoreCase(inputFileName, ".vwnod_c")) {
        result.inspectOptions.followReferences = true;
    }

    if (result.inspectOptions.followReferences
        && !result.inspectOptions.inspectExternalReferences) {
        result.error =
            "reference traversal, map entity export, and scene reconstruction require RERL decoding";
    }
    return result;
}

} // namespace vmsourceconv::cli
