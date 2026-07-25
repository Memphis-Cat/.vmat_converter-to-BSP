#pragma once

#include "inspect/data/DataInspectionResult.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace vmsourceconv::serialization::kv3 {
struct Kv3Document;
}

namespace vmsourceconv::graph {

enum class ResourceNodeStatus {
    Loaded,
    Missing,
    ParseError,
    IoError,
};

enum class ResourceNodeSource {
    None,
    LooseFile,
    VpkArchive,
};

[[nodiscard]] const char* ToString(ResourceNodeStatus status) noexcept;
[[nodiscard]] const char* ToString(ResourceNodeSource source) noexcept;

struct ResourceBlockSummary {
    std::string type;
    std::size_t offset = 0;
    std::uint32_t size = 0;
};

struct Kv3DecodeSummary {
    bool attempted = false;
    bool succeeded = false;
    std::string message;
    std::string rootType;
    std::size_t stringCount = 0;
    std::size_t nodeCount = 0;
    std::size_t objectCount = 0;
    std::size_t arrayCount = 0;
    std::size_t binaryBlobCount = 0;
    std::size_t maximumDepth = 0;
};

struct ResourceGraphNode {
    std::size_t index = 0;
    std::optional<std::size_t> parentIndex;
    std::uint64_t referenceId = 0;
    std::string logicalName;
    std::filesystem::path compiledRelativePath;
    std::filesystem::path resolvedPath;
    std::string vpkEntryPath;
    std::size_t depth = 0;
    ResourceNodeStatus status = ResourceNodeStatus::Missing;
    ResourceNodeSource source = ResourceNodeSource::None;
    std::string message;

    std::uintmax_t actualSize = 0;
    std::uint32_t declaredFileSize = 0;
    std::uint16_t headerVersion = 0;
    std::uint16_t resourceVersion = 0;
    std::vector<ResourceBlockSummary> blocks;
    inspect::data::DataInspectionResult dataInspection;
    Kv3DecodeSummary kv3Decode;
    std::filesystem::path kv3JsonPath;
    std::shared_ptr<serialization::kv3::Kv3Document> kv3Document;
    std::size_t externalReferenceCount = 0;
    std::size_t warningCount = 0;
    std::size_t errorCount = 0;
};

struct ResourceGraphStatistics {
    std::size_t referencesSeen = 0;
    std::size_t referencesSkipped = 0;
    std::size_t duplicateReferences = 0;
    std::size_t depthLimitedReferences = 0;
    std::size_t resourcesLoaded = 0;
    std::size_t resourcesLoadedLoose = 0;
    std::size_t resourcesLoadedFromVpk = 0;
    std::size_t resourcesMissing = 0;
    std::size_t resourcesFailed = 0;
    std::size_t kv3DocumentsDecoded = 0;
    std::size_t kv3DocumentsFailed = 0;
    std::size_t kv3DocumentsExported = 0;
    std::size_t entityDocumentsRetained = 0;
    std::size_t sceneDocumentsRetained = 0;
    bool resourceLimitReached = false;
};

struct ResourceGraph {
    bool enabled = false;
    bool includeAssets = false;
    std::size_t maximumDepth = 0;
    std::size_t maximumResources = 0;
    std::vector<std::filesystem::path> searchRoots;
    std::vector<std::filesystem::path> mountedVpks;
    std::vector<ResourceGraphNode> nodes;
    ResourceGraphStatistics statistics;
};

} // namespace vmsourceconv::graph
