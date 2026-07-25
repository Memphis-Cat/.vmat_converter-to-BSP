#include "inspect/JsonReportWriter.h"

#include "core/Diagnostic.h"
#include "graph/ResourceGraph.h"

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
    const std::string_view indentation) {
    output << '[';
    for (std::size_t index = 0; index < paths.size(); ++index) {
        output << (index == 0 ? "\n" : ",\n") << indentation;
        WriteEscaped(output, paths[index].string());
    }
    output << (paths.empty() ? "]" : "\n    ]");
}

void WriteResourceGraph(
    const graph::ResourceGraph& resourceGraph,
    std::ostream& output) {
    output << "{\n"
           << "    \"enabled\": "
           << (resourceGraph.enabled ? "true" : "false") << ",\n"
           << "    \"includeAssets\": "
           << (resourceGraph.includeAssets ? "true" : "false") << ",\n"
           << "    \"maximumDepth\": " << resourceGraph.maximumDepth << ",\n"
           << "    \"maximumResources\": "
           << resourceGraph.maximumResources << ",\n"
           << "    \"searchRoots\": ";
    WritePathArray(resourceGraph.searchRoots, output, "      ");

    output << ",\n    \"mountedVpks\": ";
    WritePathArray(resourceGraph.mountedVpks, output, "      ");

    output << ",\n    \"nodes\": [";
    for (std::size_t index = 0;
         index < resourceGraph.nodes.size();
         ++index) {
        const auto& node = resourceGraph.nodes[index];
        output << (index == 0 ? "\n" : ",\n")
               << "      {\n"
               << "        \"index\": " << node.index << ",\n"
               << "        \"parentIndex\": ";

        if (node.parentIndex.has_value()) {
            output << *node.parentIndex;
        } else {
            output << "null";
        }

        output << ",\n        \"referenceId\": " << node.referenceId
               << ",\n        \"logicalName\": ";
        WriteEscaped(output, node.logicalName);
        output << ",\n        \"compiledRelativePath\": ";
        WriteEscaped(output, node.compiledRelativePath.string());
        output << ",\n        \"resolvedPath\": ";
        WriteEscaped(output, node.resolvedPath.string());
        output << ",\n        \"vpkEntryPath\": ";
        WriteEscaped(output, node.vpkEntryPath);
        output << ",\n        \"depth\": " << node.depth
               << ",\n        \"status\": ";
        WriteEscaped(output, graph::ToString(node.status));
        output << ",\n        \"source\": ";
        WriteEscaped(output, graph::ToString(node.source));
        output << ",\n        \"message\": ";
        WriteEscaped(output, node.message);
        output << ",\n        \"actualSize\": " << node.actualSize
               << ",\n        \"declaredFileSize\": "
               << node.declaredFileSize
               << ",\n        \"headerVersion\": " << node.headerVersion
               << ",\n        \"resourceVersion\": "
               << node.resourceVersion
               << ",\n        \"externalReferenceCount\": "
               << node.externalReferenceCount
               << ",\n        \"warningCount\": " << node.warningCount
               << ",\n        \"errorCount\": " << node.errorCount
               << ",\n        \"blocks\": [";

        for (std::size_t blockIndex = 0;
             blockIndex < node.blocks.size();
             ++blockIndex) {
            const auto& block = node.blocks[blockIndex];
            output << (blockIndex == 0 ? "\n" : ",\n")
                   << "          {\"type\": ";
            WriteEscaped(output, block.type);
            output << ", \"offset\": " << block.offset
                   << ", \"size\": " << block.size << '}';
        }

        output << (node.blocks.empty() ? "]" : "\n        ]")
               << "\n      }";
    }
    output << (resourceGraph.nodes.empty() ? "]" : "\n    ]");

    const auto& statistics = resourceGraph.statistics;
    output << ",\n    \"statistics\": {\n"
           << "      \"referencesSeen\": "
           << statistics.referencesSeen << ",\n"
           << "      \"referencesSkipped\": "
           << statistics.referencesSkipped << ",\n"
           << "      \"duplicateReferences\": "
           << statistics.duplicateReferences << ",\n"
           << "      \"depthLimitedReferences\": "
           << statistics.depthLimitedReferences << ",\n"
           << "      \"resourcesLoaded\": "
           << statistics.resourcesLoaded << ",\n"
           << "      \"resourcesLoadedLoose\": "
           << statistics.resourcesLoadedLoose << ",\n"
           << "      \"resourcesLoadedFromVpk\": "
           << statistics.resourcesLoadedFromVpk << ",\n"
           << "      \"resourcesMissing\": "
           << statistics.resourcesMissing << ",\n"
           << "      \"resourcesFailed\": "
           << statistics.resourcesFailed << ",\n"
           << "      \"resourceLimitReached\": "
           << (statistics.resourceLimitReached ? "true" : "false")
           << "\n    }\n"
           << "  }";
}

} // namespace

void JsonReportWriter::Write(
    const InspectionReport& report,
    std::ostream& output) const {
    const auto& document = report.document;
    const auto& header = document.header;

    output << "{\n  \"file\": ";
    WriteEscaped(output, document.file.path.string());
    output << ",\n  \"actualSize\": " << document.file.bytes.size()
           << ",\n  \"header\": {\n"
           << "    \"declaredFileSize\": "
           << header.declaredFileSize << ",\n"
           << "    \"headerVersion\": "
           << header.headerVersion << ",\n"
           << "    \"resourceVersion\": "
           << header.resourceVersion << ",\n"
           << "    \"blockDirectoryRelativeOffset\": "
           << header.blockDirectoryRelativeOffset << ",\n"
           << "    \"blockDirectoryOffset\": "
           << header.blockDirectoryOffset << ",\n"
           << "    \"blockCount\": " << header.blockCount << "\n"
           << "  },\n  \"blocks\": [";

    for (std::size_t index = 0; index < document.blocks.size(); ++index) {
        const auto& block = document.blocks[index];
        output << (index == 0 ? "\n" : ",\n")
               << "    {\"index\": " << block.index << ", \"type\": ";
        WriteEscaped(output, block.type.ToString());
        output << ", \"directoryEntryOffset\": "
               << block.directoryEntryOffset
               << ", \"relativeOffset\": " << block.relativeOffset
               << ", \"offset\": " << block.offset
               << ", \"size\": " << block.size << '}';
    }
    output << (document.blocks.empty() ? "]" : "\n  ]");

    output << ",\n  \"externalReferences\": [";
    for (std::size_t index = 0;
         index < report.externalReferences.size();
         ++index) {
        const auto& reference = report.externalReferences[index];
        output << (index == 0 ? "\n" : ",\n")
               << "    {\"id\": " << reference.id << ", \"name\": ";
        WriteEscaped(output, reference.name);
        output << ", \"entryOffset\": " << reference.entryOffset
               << ", \"stringOffset\": " << reference.stringOffset
               << '}';
    }
    output << (report.externalReferences.empty() ? "]" : "\n  ]");

    output << ",\n  \"resourceGraph\": ";
    WriteResourceGraph(report.resourceGraph, output);

    output << ",\n  \"diagnostics\": [";
    for (std::size_t index = 0;
         index < report.diagnostics.size();
         ++index) {
        const auto& diagnostic = report.diagnostics[index];
        output << (index == 0 ? "\n" : ",\n")
               << "    {\"severity\": ";
        WriteEscaped(output, core::ToString(diagnostic.severity));
        output << ", \"code\": ";
        WriteEscaped(output, diagnostic.code);
        output << ", \"message\": ";
        WriteEscaped(output, diagnostic.message);
        output << ", \"offset\": ";

        if (diagnostic.offset.has_value()) {
            output << *diagnostic.offset;
        } else {
            output << "null";
        }
        output << '}';
    }

    output << (report.diagnostics.empty() ? "]" : "\n  ]")
           << ",\n  \"summary\": {\"warnings\": "
           << report.WarningCount()
           << ", \"errors\": " << report.ErrorCount() << "}\n}\n";
}

void JsonReportWriter::WriteFile(
    const InspectionReport& report,
    const std::filesystem::path& path) const {
    std::ofstream stream(path, std::ios::binary);
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
