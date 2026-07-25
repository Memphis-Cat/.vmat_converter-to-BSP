#pragma once

#include "core/Diagnostic.h"
#include "graph/ResourceGraph.h"
#include "inspect/data/DataInspectionResult.h"
#include "resource/ResourceDocument.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::inspect {

struct ExternalReference {
    std::uint64_t id = 0;
    std::string name;
    std::size_t entryOffset = 0;
    std::size_t stringOffset = 0;
};

struct InspectionReport {
    resource::ResourceDocument document;
    data::DataInspectionResult dataInspection;
    std::vector<ExternalReference> externalReferences;
    graph::ResourceGraph resourceGraph;
    std::vector<core::Diagnostic> diagnostics;

    [[nodiscard]] std::size_t WarningCount() const noexcept;
    [[nodiscard]] std::size_t ErrorCount() const noexcept;
};

} // namespace vmsourceconv::inspect
