#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace vmsourceconv::inspect {

struct InspectOptions {
    std::filesystem::path input;
    std::filesystem::path jsonOutput;
    std::filesystem::path dumpDirectory;
    std::filesystem::path logOutput;
    std::filesystem::path entitiesJsonOutput;
    std::vector<std::filesystem::path> resourceRoots;
    std::vector<std::filesystem::path> vpkPaths;
    std::size_t maximumDepth = 4;
    std::size_t maximumResources = 512;
    bool inspectExternalReferences = true;
    bool followReferences = false;
    bool includeAssets = false;
    bool inspectData = false;
    bool decodeKv3 = false;
    bool decodeEntityLumps = false;
    bool strict = false;
};

} // namespace vmsourceconv::inspect
