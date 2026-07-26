#pragma once

#include "core/Diagnostic.h"
#include "graph/ResourceGraph.h"
#include "inspect/InspectionReport.h"
#include "io/ResourceReadLimits.h"
#include "vpk/VpkVerification.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace vmsourceconv::graph {

struct ResourceGraphBuildOptions {
    std::vector<std::filesystem::path> resourceRoots;
    std::vector<std::filesystem::path> overrideRoots;
    std::vector<std::filesystem::path> vpkPaths;
    bool includeAssets = false;
    bool inspectData = false;
    bool decodeKv3 = false;
    bool decodeEntityLumps = false;
    bool reconstructScene = false;
    bool reportVpkCollisions = false;
    bool strictVpkCollisions = false;
    std::filesystem::path kv3JsonDirectory;
    std::vector<std::string> kv3Filters;
    std::size_t maximumDepth = 4;
    std::size_t maximumResources = 512;
    io::ResourceReadLimits readLimits;
    vpk::VpkReadOptions vpkReadOptions;
};

class ResourceGraphBuilder final {
public:
    [[nodiscard]] ResourceGraph Build(
        const std::filesystem::path& input,
        const std::vector<inspect::ExternalReference>& rootReferences,
        const ResourceGraphBuildOptions& options,
        std::vector<core::Diagnostic>& diagnostics) const;
};

} // namespace vmsourceconv::graph
