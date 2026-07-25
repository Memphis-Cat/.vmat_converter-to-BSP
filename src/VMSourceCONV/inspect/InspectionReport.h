#pragma once

#include "core/Diagnostic.h"
#include "entities/EntityLumpExport.h"
#include "graph/ResourceGraph.h"
#include "inspect/data/DataInspectionResult.h"
#include "resource/ResourceDocument.h"
#include "scene/WorldScene.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace vmsourceconv::serialization::kv3 {
struct Kv3Document;
}

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
    graph::Kv3DecodeSummary rootKv3Decode;
    std::filesystem::path rootKv3JsonPath;
    std::shared_ptr<serialization::kv3::Kv3Document> rootKv3Document;
    std::vector<ExternalReference> externalReferences;
    graph::ResourceGraph resourceGraph;
    std::vector<entities::EntityLumpExport> entityLumps;
    scene::WorldScene worldScene;
    std::vector<core::Diagnostic> diagnostics;

    [[nodiscard]] std::size_t WarningCount() const noexcept;
    [[nodiscard]] std::size_t ErrorCount() const noexcept;
};

} // namespace vmsourceconv::inspect
