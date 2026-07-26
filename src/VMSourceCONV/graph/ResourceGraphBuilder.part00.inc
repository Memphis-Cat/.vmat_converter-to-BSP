#include "graph/ResourceGraphBuilder.h"

#include "core/ParseError.h"
#include "discovery/ResourceIdentity.h"
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
    std::size_t sourceNode = 0;
    std::size_t edgeIndex = 0;
    inspect::ExternalReference reference;
    discovery::ResourceIdentity identity;
    std::size_t depth = 0;
    std::vector<std::string> ancestry;
};

[[nodiscard]] std::size_t CountSeverity(
    const std::vector<core::Diagnostic>& diagnostics,
    const core::DiagnosticSeverity severity) {
    return static_cast<std::size_t>(std::count_if(
        diagnostics.begin(), diagnostics.end(),
        [severity](const core::Diagnostic& diagnostic) {
            return diagnostic.severity == severity;
        }));
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

[[nodiscard]] bool EndsWithIgnoreCase(
    const std::string_view value,
    const std::string_view suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(
        suffix.rbegin(), suffix.rend(), value.rbegin(),
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
            logicalName, kv3Filters);
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
            document, *dataBlock);
        PopulateKv3Summary(decoded, node.kv3Decode);
        ++statistics.kv3DocumentsDecoded;

        if (exportSelected) {
            try {
                node.kv3JsonPath =
                    serialization::kv3::Kv3DocumentExporter::Export(
                        decoded, logicalName, kv3JsonDirectory);
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

void CheckReferenceIdentity(
    const inspect::ExternalReference& reference,
    const discovery::ResourceIdentity& identity,
    std::unordered_map<std::uint64_t, std::string>& idToKey,
    std::unordered_map<std::string, std::uint64_t>& keyToId,
    ResourceGraphStatistics& statistics,
    std::vector<core::Diagnostic>& diagnostics) {
    const auto idMatch = idToKey.find(reference.id);
    if (idMatch != idToKey.end() && idMatch->second != identity.key) {
        ++statistics.referenceIdentityConflicts;
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "RERL_ID_NAME_CONFLICT",
            "resource id " + std::to_string(reference.id)
                + " maps to both " + idMatch->second + " and " + identity.key,
            std::nullopt,
        });
    } else {
        idToKey.emplace(reference.id, identity.key);
    }

    const auto keyMatch = keyToId.find(identity.key);
    if (keyMatch != keyToId.end() && keyMatch->second != reference.id) {
        ++statistics.referenceIdentityConflicts;
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "RERL_NAME_ID_CONFLICT",
            identity.key + " maps to multiple resource ids",
            std::nullopt,
        });
    } else {
        keyToId.emplace(identity.key, reference.id);
    }
}

void EnqueueReferences(
    const std::vector<inspect::ExternalReference>& references,
    const std::size_t sourceNode,
    const std::size_t depth,
    const std::vector<std::string>& ancestry,
    const bool includeAssets,
    std::deque<PendingReference>& pending,
    ResourceGraph& graph,
    std::unordered_map<std::uint64_t, std::string>& idToKey,
    std::unordered_map<std::string, std::uint64_t>& keyToId,
    std::vector<core::Diagnostic>& diagnostics) {
    for (const auto& reference : references) {
        ++graph.statistics.referencesSeen;

        std::string identityError;
        const auto identity = discovery::ResourceIdentity::TryCreate(
            reference.name, &identityError);
        if (!identity.has_value()) {
            ++graph.statistics.invalidReferences;
            graph.edges.push_back({
                sourceNode,
                std::nullopt,
                reference.id,
                reference.name,
                {},
                DependencyDisposition::Invalid,
                identityError,
            });
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "INVALID_RESOURCE_REFERENCE",
                reference.name + ": " + identityError,
                std::nullopt,
            });
            continue;
        }

        CheckReferenceIdentity(
            reference, *identity, idToKey, keyToId,
            graph.statistics, diagnostics);

        if (!ShouldFollowResource(identity->logicalName, includeAssets)) {
            ++graph.statistics.referencesSkipped;
            graph.edges.push_back({
                sourceNode,
                std::nullopt,
                reference.id,
                identity->logicalName,
                identity->key,
                DependencyDisposition::Skipped,
                "dependency filter skipped this resource",
            });
            continue;
        }

        const auto edgeIndex = graph.edges.size();
        graph.edges.push_back({
            sourceNode,
            std::nullopt,
            reference.id,
            identity->logicalName,
            identity->key,
            DependencyDisposition::Resolved,
            {},
        });
        pending.push_back({
            sourceNode,
            edgeIndex,
            reference,
            *identity,
            depth,
            ancestry,
        });
    }
}

} // namespace
