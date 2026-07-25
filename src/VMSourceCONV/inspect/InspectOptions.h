#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace vmsourceconv::inspect {

struct InspectOptions {
    std::filesystem::path input;
    std::filesystem::path jsonOutput;
    std::filesystem::path dumpDirectory;
    std::vector<std::filesystem::path> resourceRoots;
    std::size_t maximumDepth = 4;
    std::size_t maximumResources = 512;
    bool inspectExternalReferences = true;
    bool followReferences = false;
    bool includeAssets = false;
    bool strict = false;
};

} // namespace vmsourceconv::inspect
