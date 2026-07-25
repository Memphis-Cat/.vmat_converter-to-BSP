#include "inspect/Inspector.h"

#include "entities/EntityLumpExport.h"
#include "graph/ResourceGraphBuilder.h"
#include "inspect/RerlInspector.h"
#include "inspect/data/DataBlockInspector.h"
#include "io/FileReader.h"
#include "resource/ResourceParser.h"
#include "scene/WorldSceneExtractor.h"
#include "serialization/kv3/BinaryKv3Decoder.h"
#include "serialization/kv3/Kv3Document.h"
#include "serialization/kv3/Kv3DocumentExporter.h"
#include "serialization/kv3/Kv3Value.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace vmsourceconv::inspect {
namespace {

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
    graph::Kv3DecodeSummary& summary) {
    summary.attempted = true;
    summary.succeeded = true;
    summary.rootType = serialization::kv3::ToString(document.root.Type());
    summary.stringCount = document.statistics.stringCount;
    summary.nodeCount = document.statistics.nodeCount;
    summary.objectCount = document.statistics.objectCount;
    summary.arrayCount = document.statistics.arrayCount;
    summary.binaryBlobCount = document.statistics.binaryBlobCount;
    summary.maximumDepth = document.statistics.maximumDepth;
}

void ExtractEntityLump(
    const std::string& logicalName,
    const serialization::kv3::Kv3Document& document,
    InspectionReport& report) {
    try {
        report.entityLumps.push_back(
            entities::EntityLumpExtractor{}.Extract(logicalName, document));
    } catch (const std::exception& error) {
        report.diagnostics.push_back({
            core::DiagnosticSeverity::Error,
            "ENTITY_LUMP_DECODE_FAILED",
            logicalName + ": " + error.what(),
            std::nullopt,
        });
    }
}

void AddSceneDocument(
    const std::string& logicalName,
    const serialization::kv3::Kv3Document& document,
    InspectionReport& report) {
    try {
        scene::WorldSceneExtractor{}.AddDocument(
            logicalName,
            document,
            report.worldScene);
    } catch (const std::exception& error) {
        report.diagnostics.push_back({
            core::DiagnosticSeverity::Error,
            "WORLD_SCENE_DECODE_FAILED",
            logicalName + ": " + error.what(),
            std::nullopt,
        });
    }
}

} // namespace

InspectionReport Inspector::Run(const InspectOptions& options) const {
    InspectionReport report;
    auto file = io::FileReader::ReadAll(options.input);
    report.document = resource::ResourceParser{}.Parse(
        std::move(file),
        report.diagnostics);

    if (options.inspectExternalReferences) {
        report.externalReferences = RerlInspector{}.Inspect(
            report.document,
            report.diagnostics);
    }

    if (options.inspectData || options.decodeKv3 || options.decodeEntityLumps
        || options.reconstructScene || !options.kv3JsonDirectory.empty()) {
        report.dataInspection = data::DataBlockInspector{}.Inspect(report.document);
    }

    const auto rootLogicalName = options.input.generic_string();
    const auto rootFileName = options.input.filename().string();
    const auto rootIsEntityLump = EndsWithIgnoreCase(rootFileName, ".vents_c");
    const auto rootIsScene = scene::WorldSceneExtractor::IsWorldResource(rootFileName)
        || scene::WorldSceneExtractor::IsWorldNodeResource(rootFileName);
    const auto exportRoot = !options.kv3JsonDirectory.empty()
        && serialization::kv3::Kv3DocumentExporter::Matches(
            rootLogicalName,
            options.kv3Filters);
    const auto decodeRoot = options.decodeKv3
        || (options.decodeEntityLumps && rootIsEntityLump)
        || (options.reconstructScene && rootIsScene)
        || exportRoot;

    if (decodeRoot
        && report.dataInspection.format == data::DataFormat::BinaryKv3) {
        report.rootKv3Decode.attempted = true;
        if (const auto* dataBlock = FindDataBlock(report.document)) {
            try {
                auto document = serialization::kv3::BinaryKv3Decoder{}.Decode(
                    report.document,
                    *dataBlock);
                PopulateKv3Summary(document, report.rootKv3Decode);

                if (exportRoot) {
                    try {
                        report.rootKv3JsonPath =
                            serialization::kv3::Kv3DocumentExporter::Export(
                                document,
                                rootLogicalName,
                                options.kv3JsonDirectory);
                    } catch (const std::exception& error) {
                        report.diagnostics.push_back({
                            core::DiagnosticSeverity::Error,
                            "ROOT_KV3_JSON_WRITE_FAILED",
                            error.what(),
                            std::nullopt,
                        });
                    }
                }

                if ((options.decodeEntityLumps && rootIsEntityLump)
                    || (options.reconstructScene && rootIsScene)) {
                    report.rootKv3Document =
                        std::make_shared<serialization::kv3::Kv3Document>(
                            std::move(document));
                }
            } catch (const std::exception& error) {
                report.rootKv3Decode.message = error.what();
                report.diagnostics.push_back({
                    core::DiagnosticSeverity::Warning,
                    "ROOT_KV3_DECODE_FAILED",
                    error.what(),
                    dataBlock->offset,
                });
            }
        }
    }

    if (options.followReferences) {
        report.resourceGraph = graph::ResourceGraphBuilder{}.Build(
            options.input,
            report.externalReferences,
            options.resourceRoots,
            options.vpkPaths,
            options.includeAssets,
            options.inspectData,
            options.decodeKv3,
            options.decodeEntityLumps,
            options.reconstructScene,
            options.kv3JsonDirectory,
            options.kv3Filters,
            options.maximumDepth,
            options.maximumResources,
            report.diagnostics);
    }

    if (options.decodeEntityLumps) {
        if (rootIsEntityLump && report.rootKv3Document) {
            ExtractEntityLump(
                rootLogicalName,
                *report.rootKv3Document,
                report);
        }
        for (const auto& node : report.resourceGraph.nodes) {
            if (node.kv3Document
                && EndsWithIgnoreCase(node.logicalName, ".vents")) {
                ExtractEntityLump(
                    node.logicalName,
                    *node.kv3Document,
                    report);
            }
        }

        if (report.entityLumps.empty()) {
            report.diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "ENTITY_LUMP_NOT_FOUND",
                "no decodable entity lump was found",
                std::nullopt,
            });
        }
    }

    if (options.reconstructScene) {
        if (rootIsScene && report.rootKv3Document) {
            AddSceneDocument(
                rootLogicalName,
                *report.rootKv3Document,
                report);
        }
        for (const auto& node : report.resourceGraph.nodes) {
            if (node.kv3Document
                && (scene::WorldSceneExtractor::IsWorldResource(node.logicalName)
                    || scene::WorldSceneExtractor::IsWorldNodeResource(
                        node.logicalName))) {
                AddSceneDocument(
                    node.logicalName,
                    *node.kv3Document,
                    report);
            }
        }
        scene::WorldSceneExtractor{}.Finalize(report.worldScene);
        if (report.worldScene.worlds.empty()
            && report.worldScene.worldNodes.empty()) {
            report.diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "SCENE_DOCUMENT_NOT_FOUND",
                "no decodable world or world-node document was found",
                std::nullopt,
            });
        }
    }

    return report;
}

} // namespace vmsourceconv::inspect
