#include "graph/DependencyFilter.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <string>

namespace vmsourceconv::graph {
namespace {

[[nodiscard]] std::string Lowercase(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

} // namespace

bool IsStructuralMapResource(const std::string_view logicalName) {
    const auto lowered = Lowercase(std::string(logicalName));
    const std::filesystem::path path(lowered);
    const auto extension = path.extension().string();

    constexpr std::array<std::string_view, 8> structuralExtensions{
        ".vmap",
        ".vrman",
        ".vwrld",
        ".vwnod",
        ".vents",
        ".vvis",
        ".vphys",
        ".vdata",
    };

    if (std::find(
            structuralExtensions.begin(),
            structuralExtensions.end(),
            extension) != structuralExtensions.end()) {
        return true;
    }

    return extension == ".vmdl"
        && lowered.find("world_physics") != std::string::npos;
}

bool ShouldFollowResource(
    const std::string_view logicalName,
    const bool includeAssets) {
    return includeAssets || IsStructuralMapResource(logicalName);
}

} // namespace vmsourceconv::graph
