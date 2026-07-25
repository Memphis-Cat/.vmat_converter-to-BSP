#include "inspect/JsonReportWriter.h"

#include "core/Diagnostic.h"
#include "graph/ResourceGraph.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace vmsourceconv::inspect {
namespace {

void WriteEscaped(std::ostream& output, const std::string_view value) {
    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20U) {
                    output << "\\u" << std::hex << std::setw(4)
                           << std::setfill('0')
                           << static_cast<unsigned int>(character)
                           << std::dec << std::setfill(' ');
                } else {
                    output << static_cast<char>(character);
                }
                break;
        }
    }
    output << '"';
}

void WritePathArray(
    const std::vector<std::filesystem::path>& paths,
    std::ostream& output,
    const std::string_view itemIndent,
    const std::string_view closingIndent) {
    output << '[';
    for (std::size_t index = 0; index < paths.size(); ++index) {
        output << (index == 0U ? "\n" : ",\n") << itemIndent;
        WriteEscaped(output, paths[index].generic_string());
    }
    if (!paths.empty()) {
        output << '\n' << closingIndent;
    }
    output << ']';
}

void WriteResourceGraph(
    const graph::ResourceGraph& graph,
    std::ostream& output) {
    output << "{\n"
           << "    \"enabled\": " << (graph.enabled ? "true" : "false") << ",\n"
           << "    \"includeAssets\": " << (graph.includeAssets ? "true" : "false") << ",\n"
           << "    \"maximumDepth\": " << graph.maximumDepth << ",\n"
           << "    \"maximumResources\": " << graph.maximumResources << ",\n"
           << "    \"maximumGraphBytes\": " << graph.maximumGraphBytes << ",\n"
           << "    \"overrideRoots\": ";
    WritePathArray(graph.overrideRoots, output, "      ", "    ");
    output << ",\n    \"searchRoots\": ";
    WritePathArray(graph.searchRoots, output, "      ", "    ");
    output << ",\n    \"mountedVpks\": ";
    WritePathArray(graph.mountedVpks, output, "      ", "    ");

    output << ",\n    \"nodes\": [";
    for (std::size_t index = 0; index < graph.nodes.size(); ++index) {
        const auto& node = graph.nodes[index];
        output << (index == 0U ? "\n" : ",\n")
               << "      {\n        \"index\": " << node.index
               << ",\n        \"parentIndex\": ";
        if (node.parentIndex.has_value()) output << *node.parentIndex;
        else output << "null";
        output << ",\n        \"referenceId\": " << node.referenceId
               << ",\n        \"logicalName\": ";
        WriteEscaped(output, node.logicalName);
        output << ",\n        \"identityKey\": ";
        WriteEscaped(output, node.identityKey);
        output << ",\n        \"compiledRelativePath\": ";
        WriteEscaped(output, node.compiledRelativePath.generic_string());
        output << ",\n        \"resolvedPath\": ";
        WriteEscaped(output, node.resolvedPath.generic_string());
        output << ",\n        \"vpkEntryPath\": ";
        WriteEscaped(output, node.vpkEntryPath);
        output << ",\n        \"lookupTier\": ";
        WriteEscaped(output, node.lookupTier);
        output << ",\n        \"shadowedVpks\": ";
        WritePathArray(node.shadowedVpks, output, "          ", "        ");
        output << ",\n        \"depth\": " << node.depth
               << ",\n        \"status\": ";
        WriteEscaped(output, graph::ToString(node.status));
        output << ",\n        \"source\": ";
        WriteEscaped(output, graph::ToString(node.source));
        output << ",\n        \"message\": ";
        WriteEscaped(output, node.message);
        output << ",\n        \"actualSize\": " << node.actualSize
               << ",\n        \"declaredFileSize\": " << node.declaredFileSize
               << ",\n        \"headerVersion\": " << node.headerVersion
               << ",\n        \"resourceVersion\": " << node.resourceVersion
               << ",\n        \"externalReferenceCount\": " << node.externalReferenceCount
               << ",\n        \"warningCount\": " << node.warningCount
               << ",\n        \"errorCount\": " << node.errorCount
               << ",\n        \"blocks\": [";
        for (std::size_t blockIndex = 0; blockIndex < node.blocks.size(); ++blockIndex) {
            const auto& block = node.blocks[blockIndex];
            output << (blockIndex == 0U ? "\n" : ",\n")
                   << "          {\"type\": ";
            WriteEscaped(output, block.type);
            output << ", \"offset\": " << block.offset
                   << ", \"size\": " << block.size << '}';
        }
        output << (node.blocks.empty() ? "]" : "\n        ]")
               << "\n      }";
    }
    output << (graph.nodes.empty() ? "]" : "\n    ]");

    output << ",\n    \"edges\": [";
    for (std::size_t index = 0; index < graph.edges.size(); ++index) {
        const auto& edge = graph.edges[index];
        output << (index == 0U ? "\n" : ",\n")
               << "      {\"sourceNode\": " << edge.sourceNode
               << ", \"targetNode\": ";
        if (edge.targetNode.has_value()) output << *edge.targetNode;
        else output << "null";
        output << ", \"referenceId\": " << edge.referenceId
               << ", \"logicalName\": ";
        WriteEscaped(output, edge.logicalName);
        output << ", \"identityKey\": ";
        WriteEscaped(output, edge.identityKey);
        output << ", \"disposition\": ";
        WriteEscaped(output, graph::ToString(edge.disposition));
        output << ", \"message\": ";
        WriteEscaped(output, edge.message);
        output << '}';
    }
    output << (graph.edges.empty() ? "]" : "\n    ]");

    output << ",\n    \"vpkCollisions\": [";
    for (std::size_t index = 0; index < graph.vpkCollisions.size(); ++index) {
        const auto& collision = graph.vpkCollisions[index];
        output << (index == 0U ? "\n" : ",\n")
               << "      {\"logicalName\": ";
        WriteEscaped(output, collision.logicalName);
        output << ", \"selectedPath\": ";
        WriteEscaped(output, collision.selectedPath.generic_string());
        output << ", \"selectedTier\": ";
        WriteEscaped(output, collision.selectedTier);
        output << ", \"shadowedPaths\": ";
        WritePathArray(collision.shadowedPaths, output, "        ", "      ");
        output << '}';
    }
    output << (graph.vpkCollisions.empty() ? "]" : "\n    ]");

    const auto& s = graph.statistics;
    output << ",\n    \"statistics\": {\n"
           << "      \"referencesSeen\": " << s.referencesSeen << ",\n"
           << "      \"referencesSkipped\": " << s.referencesSkipped << ",\n"
           << "      \"repeatedEdges\": " << s.repeatedEdges << ",\n"
           << "      \"cycleEdges\": " << s.cycleEdges << ",\n"
           << "      \"selfReferences\": " << s.selfReferences << ",\n"
           << "      \"invalidReferences\": " << s.invalidReferences << ",\n"
           << "      \"referenceIdentityConflicts\": " << s.referenceIdentityConflicts << ",\n"
           << "      \"vpkCollisionCount\": " << s.vpkCollisionCount << ",\n"
           << "      \"depthLimitedReferences\": " << s.depthLimitedReferences << ",\n"
           << "      \"resourcesLoaded\": " << s.resourcesLoaded << ",\n"
           << "      \"resourcesLoadedLoose\": " << s.resourcesLoadedLoose << ",\n"
           << "      \"resourcesLoadedFromVpk\": " << s.resourcesLoadedFromVpk << ",\n"
           << "      \"resourcesMissing\": " << s.resourcesMissing << ",\n"
           << "      \"resourcesFailed\": " << s.resourcesFailed << ",\n"
           << "      \"totalBytesLoaded\": " << s.totalBytesLoaded << ",\n"
           << "      \"resourceLimitReached\": " << (s.resourceLimitReached ? "true" : "false") << ",\n"
           << "      \"graphByteLimitReached\": " << (s.graphByteLimitReached ? "true" : "false") << "\n"
           << "    }\n  }";
}

} // namespace

void JsonReportWriter::Write(
    const InspectionReport& report,
    std::ostream& output) const {
    const auto& document = report.document;
    const auto& header = document.header;
    output << "{\n  \"file\": ";
    WriteEscaped(output, document.file.path.generic_string());
    output << ",\n  \"actualSize\": " << document.file.bytes.size()
           << ",\n  \"header\": {\n"
           << "    \"declaredFileSize\": " << header.declaredFileSize << ",\n"
           << "    \"headerVersion\": " << header.headerVersion << ",\n"
           << "    \"resourceVersion\": " << header.resourceVersion << ",\n"
           << "    \"blockDirectoryRelativeOffset\": " << header.blockDirectoryRelativeOffset << ",\n"
           << "    \"blockDirectoryOffset\": " << header.blockDirectoryOffset << ",\n"
           << "    \"blockCount\": " << header.blockCount << "\n  },\n"
           << "  \"blocks\": [";
    for (std::size_t index = 0; index < document.blocks.size(); ++index) {
        const auto& block = document.blocks[index];
        output << (index == 0U ? "\n" : ",\n")
               << "    {\"index\": " << block.index << ", \"type\": ";
        WriteEscaped(output, block.type.ToString());
        output << ", \"directoryEntryOffset\": " << block.directoryEntryOffset
               << ", \"relativeOffset\": " << block.relativeOffset
               << ", \"offset\": " << block.offset
               << ", \"size\": " << block.size << '}';
    }
    output << (document.blocks.empty() ? "]" : "\n  ]")
           << ",\n  \"externalReferences\": [";
    for (std::size_t index = 0; index < report.externalReferences.size(); ++index) {
        const auto& reference = report.externalReferences[index];
        output << (index == 0U ? "\n" : ",\n")
               << "    {\"id\": " << reference.id << ", \"name\": ";
        WriteEscaped(output, reference.name);
        output << ", \"entryOffset\": " << reference.entryOffset
               << ", \"stringOffset\": " << reference.stringOffset << '}';
    }
    output << (report.externalReferences.empty() ? "]" : "\n  ]")
           << ",\n  \"resourceGraph\": ";
    WriteResourceGraph(report.resourceGraph, output);
    output << ",\n  \"diagnostics\": [";
    for (std::size_t index = 0; index < report.diagnostics.size(); ++index) {
        const auto& diagnostic = report.diagnostics[index];
        output << (index == 0U ? "\n" : ",\n")
               << "    {\"severity\": ";
        WriteEscaped(output, core::ToString(diagnostic.severity));
        output << ", \"code\": ";
        WriteEscaped(output, diagnostic.code);
        output << ", \"message\": ";
        WriteEscaped(output, diagnostic.message);
        output << ", \"offset\": ";
        if (diagnostic.offset.has_value()) output << *diagnostic.offset;
        else output << "null";
        output << '}';
    }
    output << (report.diagnostics.empty() ? "]" : "\n  ]")
           << ",\n  \"summary\": {\"warnings\": " << report.WarningCount()
           << ", \"errors\": " << report.ErrorCount() << "}\n}\n";
}

void JsonReportWriter::WriteFile(
    const InspectionReport& report,
    const std::filesystem::path& path) const {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error(
            "unable to open JSON output file: " + path.string());
    }
    Write(report, stream);
    if (!stream) {
        throw std::runtime_error(
            "unable to write JSON output file: " + path.string());
    }
}

} // namespace vmsourceconv::inspect
