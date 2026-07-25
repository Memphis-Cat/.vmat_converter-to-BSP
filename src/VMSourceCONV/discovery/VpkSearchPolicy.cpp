#include "discovery/VpkSearchPolicy.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <string_view>
#include <system_error>

namespace vmsourceconv::discovery {
namespace {

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

std::filesystem::path NormalizeAbsolute(
    const std::filesystem::path& path) {
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    return (error ? path : absolute).lexically_normal();
}

std::string PathKey(const std::filesystem::path& path) {
    auto key = NormalizeAbsolute(path).generic_string();
#ifdef _WIN32
    std::transform(
        key.begin(),
        key.end(),
        key.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
#endif
    return key;
}

void AddUnique(
    std::vector<std::filesystem::path>& paths,
    const std::filesystem::path& path) {
    if (path.empty()) {
        return;
    }

    const auto normalized = NormalizeAbsolute(path);
    const auto key = PathKey(normalized);
    const auto duplicate = std::find_if(
        paths.begin(),
        paths.end(),
        [&key](const std::filesystem::path& existing) {
            return PathKey(existing) == key;
        });
    if (duplicate == paths.end()) {
        paths.push_back(normalized);
    }
}

} // namespace

std::string InputPackageStem(const std::filesystem::path& input) {
    auto filename = input.filename().string();
    constexpr std::array<std::string_view, 7> suffixes{
        ".vmap_c",
        ".vmat_c",
        ".vwrld_c",
        ".vwnod_c",
        ".vents_c",
        ".vmdl_c",
        ".vdata_c",
    };

    for (const auto suffix : suffixes) {
        if (EndsWithIgnoreCase(filename, suffix)) {
            filename.resize(filename.size() - suffix.size());
            return filename;
        }
    }

    if (EndsWithIgnoreCase(filename, "_c")) {
        filename.resize(filename.size() - 2U);
    }
    return std::filesystem::path(filename).stem().string();
}

std::vector<std::filesystem::path> PrimaryPackageCandidates(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& roots) {
    std::vector<std::filesystem::path> candidates;
    const auto stem = InputPackageStem(input);
    if (stem.empty()) {
        return candidates;
    }

    for (const auto& root : roots) {
        AddUnique(candidates, root / (stem + ".vpk"));
        AddUnique(candidates, root / (stem + "_dir.vpk"));
    }
    return candidates;
}

std::vector<std::filesystem::path> FallbackVpkDirectories(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& roots) {
    std::vector<std::filesystem::path> directories;
    AddUnique(directories, input.parent_path() / "vpk");

    std::error_code error;
    const auto current = std::filesystem::current_path(error);
    if (!error) {
        AddUnique(directories, current / "vpk");
    }

    for (const auto& root : roots) {
        AddUnique(directories, root / "vpk");
    }
    return directories;
}

} // namespace vmsourceconv::discovery
