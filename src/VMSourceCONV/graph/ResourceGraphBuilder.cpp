#include "graph/ResourceGraphBuilder.h"

#include "core/ParseError.h"
#include "discovery/CompiledResourcePath.h"
#include "discovery/ResourceLocator.h"
#include "graph/DependencyFilter.h"
#include "inspect/RerlInspector.h"
#include "inspect/data/DataBlockInspector.h"
#include "resource/ResourceParser.h"
#include "scene/WorldSceneExtractor.h"
#include "serialization/kv3/BinaryKv3Decoder.h"
#include "serialization/kv3/Kv3Document.h"
#include "serialization/kv3/Kv3DocumentExporter.h"
#include "serialization/kv3/Kv3Value.h"

#include <algorithm>
#include <cctype>
#include <deque>
#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vmsourceconv::graph {
namespace {

struct PendingReference {
    std::optional<std::size_t> parentIndex;
    inspect::ExternalReference reference;
    std::size_t depth = 0;
};

[[nodiscard]] std::size_t CountSeverity(
    const std::vector<core::Diagnostic>& diagnostics,
    const core::DiagnosticSeverity severity) {
    std::size_t count = 0;
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == severity) {
            ++count;
        }
    }
    return count;
}

void AppendDiagnostics(
    const std::string& logicalName,
    const std::vector<core::Diagnostic>& source,
    std::vector<core::Diagnostic>& destination) {
    for (auto diagnostic : source) {
        diagnostic.message = logicalName + ": " + diagnostic.message;
        destination.push_back(std::move(diagnostic));
    }
}

void EnqueueReferences(
    const std::vector<inspect::ExternalReference>& references,
    const std::optional<std::size_t> parentIndex,
    const std::size_t depth,
    const bool includeAssets,
    std::deque<PendingReference>& pending,
    ResourceGraphStatistics& statistics) {
    for (const auto& reference : references) {
        ++statistics.referencesSeen;
        if (!ShouldFollowResource(reference.name, includeAssets)) {
            ++statistics.referencesSkipped;
            continue;
        }
        pending.push_back({parentIndex, reference, depth});
    }
}

[[nodiscard]] bool EndsWithIgnoreCase(
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

[[nodiscard]] const resource::ResourceBlock* FindDataBlock(
    const resource::ResourceDocument& document) {
    for (const auto& block : document.blocks) {
        if (block.type.ToString() == "DATA") {
            return &block;
        }
    }
    return nullptr;
}

void PopulateKv3Summary(
    const serialization::kv3::Kv3Document& document,
    Kv3DecodeSummary& summary) {
    summary.succeeded = true;
    summary.rootType = serialization::kv3::ToString(document.root.Type());
    summary.stringCount = document.statistics.stringCount;
    summary.nodeCount = document.statistics.nodeCount;
    summary.objectCount = document.statistics.objectCount;
    summary.arrayCount = document.statistics.arrayCount;
    summary.binaryBlobCount = document.statistics.binaryBlobCount;
    summary.maximumDepth = document.statistics.maximumDepth;
}

void DecodeKv3IfRequested(
    const std::string& logicalName,
    const bool decodeKv3,
    const bool decodeEntityLumps,
    const bool reconstructScene,
    const std::filesystem::path& kv3JsonDirectory,
    const std::vector<std::string>& kv3Filters,
    const resource::ResourceDocument& document,
    ResourceGraphNode& node,
    ResourceGraphStatistics& statistics,
    std::vector<core::Diagnostic>& diagnostics) {
    const auto isEntityLump = EndsWithIgnoreCase(logicalName, ".vents");
    const auto isSceneDocument = scene::WorldSceneExtractor::IsWorldResource(logicalName)
        || scene::WorldSceneExtractor::IsWorldNodeResource(logicalName);
    const auto exportSelected = !kv3JsonDirectory.empty()
        && serialization::kv3::Kv3DocumentExporter::Matches(
            logicalName,
            kv3Filters);
    const auto shouldDecode = decodeKv3
        || (decodeEntityLumps && isEntityLump)
        || (reconstructScene && isSceneDocument)
        || exportSelected;
    if (!shouldDecode) {
        return;
    }

    if (node.dataInspection.format != inspect::data::DataFormat::BinaryKv3) {
        if ((decodeEntityLumps && isEntityLump)
            || (reconstructScene && isSceneDocument)
            || exportSelected) {
            node.kv3Decode.attempted = true;
            node.kv3Decode.message = "DATA is not Binary KV3";
            ++statistics.kv3DocumentsFailed;
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "KV3_DECODE_SKIPPED",
                logicalName + ": " + node.kv3Decode.message,
                std::nullopt,
            });
        }
        return;
    }

    node.kv3Decode.attempted = true;
    const auto* dataBlock = FindDataBlock(document);
    if (dataBlock == nullptr) {
        node.kv3Decode.message = "resource has no DATA block";
        ++statistics.kv3DocumentsFailed;
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "KV3_DECODE_SKIPPED",
            logicalName + ": " + node.kv3Decode.message,
            std::nullopt,
        });
        return;
    }

    try {
        auto decoded = serialization::kv3::BinaryKv3Decoder{}.Decode(
            document,
            *dataBlock);
        PopulateKv3Summary(decoded, node.kv3Decode);
        ++statistics.kv3DocumentsDecoded;

        if (exportSelected) {
            try {
                node.kv3JsonPath =
                    serialization::kv3::Kv3DocumentExporter::Export(
                        decoded,
                        logicalName,
                        kv3JsonDirectory);
                ++statistics.kv3DocumentsExported;
            } catch (const std::exception& error) {
                diagnostics.push_back({
                    core::DiagnosticSeverity::Error,
                    "KV3_JSON_WRITE_FAILED",
                    logicalName + ": " + error.what(),
                    std::nullopt,
                });
            }
        }

        const auto retainEntity = decodeEntityLumps && isEntityLump;
        const auto retainScene = reconstructScene && isSceneDocument;
        if (retainEntity || retainScene) {
            node.kv3Document =
                std::make_shared<serialization::kv3::Kv3Document>(
                    std::move(decoded));
            if (retainEntity) {
                ++statistics.entityDocumentsRetained;
            }
            if (retainScene) {
                ++statistics.sceneDocumentsRetained;
            }
        }
    } catch (const std::exception& error) {
        node.kv3Decode.message = error.what();
        ++statistics.kv3DocumentsFailed;
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "KV3_DECODE_FAILED",
            logicalName + ": " + error.what(),
            dataBlock->offset,
        });
    }
}

} // namespace

ResourceGraph ResourceGraphBuilder::Build(
    const std::filesystem::path& input,
    const std::vector<inspect::ExternalReference>& rootReferences,
    const std::vector<std::filesystem::path>& resourceRoots,
    const std::vector<std::filesystem::path>& vpkPaths,
    const bool includeAssets,
    const bool inspectData,
    const bool decodeKv3,
    const bool decodeEntityLumps,
    const bool reconstructScene,
    const std::filesystem::path& kv3JsonDirectory,
    const std::vector<std::string>& kv3Filters,
    const std::size_t maximumDepth,
    const std::size_t maximumResources,
    std::vector<core::Diagnostic>& diagnostics) const {
    ResourceGraph graph;
    graph.enabled = true;
    graph.includeAssets = includeAssets;
    graph.maximumDepth = maximumDepth;
    graph.maximumResources = maximumResources;

    const discovery::ResourceLocator locator(input, resourceRoots, vpkPaths);
    graph.searchRoots = locator.SearchRoots();
    graph.mountedVpks = locator.MountedVpks();

    for (const auto& warning : locator.VpkWarnings()) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "VPK_MOUNT_WARNING",
            warning,
            std::nullopt,
        });
    }

    std::deque<PendingReference> pending;
    EnqueueReferences(
        rootReferences,
        std::nullopt,
        1,
        includeAssets,
        pending,
        graph.statistics);
    std::unordered_map<std::string, std::size_t> visited;

    while (!pending.empty()) {
        auto current = std::move(pending.front());
        pending.pop_front();
        if (current.depth > maximumDepth) {
            ++graph.statistics.depthLimitedReferences;
            continue;
        }

        const auto logicalName =
            discovery::NormalizeLogicalResourceName(current.reference.name);
        if (visited.find(logicalName) != visited.end()) {
            ++graph.statistics.duplicateReferences;
            continue;
        }
        if (graph.nodes.size() >= maximumResources) {
            graph.statistics.resourceLimitReached = true;
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "RESOURCE_LIMIT_REACHED",
                "dependency traversal stopped after reaching the resource limit",
                std::nullopt,
            });
            break;
        }

        ResourceGraphNode node;
        node.index = graph.nodes.size();
        node.parentIndex = current.parentIndex;
        node.referenceId = current.reference.id;
        node.logicalName = logicalName;
        node.depth = current.depth;

        const auto location = locator.Locate(logicalName);
        node.compiledRelativePath = location.compiledRelativePath;
        visited.emplace(logicalName, node.index);
        if (!location.Found()) {
            node.status = ResourceNodeStatus::Missing;
            node.message = location.error;
            ++graph.statistics.resourcesMissing;
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "DEPENDENCY_MISSING",
                logicalName + ": " + location.error,
                std::nullopt,
            });
            graph.nodes.push_back(std::move(node));
            continue;
        }

        node.resolvedPath = location.resolvedPath;
        if (location.source == discovery::ResourceSource::LooseFile) {
            node.source = ResourceNodeSource::LooseFile;
        } else if (location.source == discovery::ResourceSource::VpkArchive) {
            node.source = ResourceNodeSource::VpkArchive;
            node.vpkEntryPath = location.vpkMatch.entryPath;
        }

        std::vector<inspect::ExternalReference> childReferences;
        try {
            std::vector<core::Diagnostic> localDiagnostics;
            auto file = locator.Read(location);
            auto document = resource::ResourceParser{}.Parse(
                std::move(file),
                localDiagnostics);
            childReferences = inspect::RerlInspector.{}.Inspect(
                document,
                localDiagnostics);

            if (inspectData || decodeKv3 || decodeEntityLumps
                || reconstructScene || !kv3JsonDirectory.empty()) {
                node.dataInspection =
                    inspect::data::DataBlockInspector{}.Inspect(document);
            }
            DecodeKv3IfRequested(
                logicalName,
                decodeKv3,
                decodeEntityLumps,
                reconstructScene,
                kv3JsonDirectory,
                kv3Filters,
                document,
                node,
                graph.statistics,
                diagnostics);

            node.status = ResourceNodeStatus::Loaded;
            node.actualSize = document.file.bytes.size();
            node.declaredFileSize = document.header.declaredFileSize;
            node.headerVersion = document.header.headerVersion;
            node.resourceVersion = document.header.resourceVersion;
            node.externalReferenceCount = childReferences.size();
            node.warningCount = CountSeverity(
                localDiagnostics,
                core::DiagnosticSeverity::Warning);
            node.errorCount = CountSeverity(
                localDiagnostics,
                core::DiagnosticSeverity::Error);
            node.blocks.reserve(document.blocks.size());
            for (const auto& block : document.blocks) {
                node.blocks.push_back({
                    block.type.ToString(),
                    block.offset,
                    block.size,
                });
            }

            AppendDiagnostics(logicalName, localDiagnostics, diagnostics);
            ++graph.statistics.resourcesLoaded;
            if (node.source == ResourceNodeSource::VpkArchive) {
                ++graph.statistics.resourcesLoadedFromVpk;
            } else {
                ++graph.statistics.resourcesLoadedLoose;
            }
        } catch (const core::ParseError& error) {
            node.status = ResourceNodeStatus::ParseError;
            node.message = error.what();
            node.errorCount = 1;
            ++graph.statistics.resourcesFailed;
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "DEPENDENCY_PARSE_ERROR",
                logicalName + ": " + error.what(),
                error.Offset(),
            });
        } catch (const std::runtime_error& error) {
            node.status = ResourceNodeStatus::IoError;
            node.message = error.what();
            node.errorCount = 1;
            ++graph.statistics.resourcesFailed;
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "DEPENDENCY_IO_ERROR",
                logicalName + ": " + error.what(),
                std::nullopt,
            });
        }

        const auto nodeIndex = node.index;
        const auto loaded = node.status == ResourceNodeStatus::Loaded;
        graph.nodes.push_back(std::move(node));
        if (loaded && current.depth < maximumDepth) {
            EnqueueReferences(
                childReferences,
                nodeIndex,
                current.depth + 1,
                includeAssets,
                pending,
                graph.statistics);
        } else if (loaded && !childReferences.empty()) {
            graph.statistics.depthLimitedReferences += childReferences.size();
        }
    }

    return graph;
}

} // namespace vmsourceconv::graph
