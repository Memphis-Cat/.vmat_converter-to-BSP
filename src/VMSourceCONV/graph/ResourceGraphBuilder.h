#pragma once

#include "core/Diagnostic.h"
#include "graph/ResourceGraph.h"
#include "inspect/InspectionReport.h"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace vmsourceconv::graph {

class ResourceGraphBuilder final {
public:
    [[nodiscard]] ResourceGraph Build(
        const std::filesystem::path& input,
        const std::vector<inspect::ExternalReference>& rootReferences,
        const std::vector<std::filesystem::path>& resourceRoots,
        const std::vector<std::filesystem::path>& vpkPaths,
        bool includeAssets,
        std::size_t maximumDepth,
        std::size_t maximumResources,
        std::vector<core::Diagnostic>& diagnostics) const;
};

} // namespace vmsourceconv::graph
