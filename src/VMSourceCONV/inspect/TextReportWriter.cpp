#include "inspect/TextReportWriter.h"

#include "core/Diagnostic.h"
#include "graph/ResourceGraph.h"
#include "inspect/data/DataInspectionResult.h"

#include <iomanip>
#include <ostream>

namespace vmsourceconv::inspect {
namespace {

void WriteHex(std::ostream& output, const std::size_t value) {
    output << "0x" << std::uppercase << std::hex << value
           << std::dec << std::nouppercase;
}

void WriteDataSummary(
    std::ostream& output,
    const data::DataInspectionResult& result,
    const char* prefix) {
    if (!result.present) {
        return;
    }

    output << prefix << "DATA format=" << data::ToString(result.format)
           << " offset=";
    WriteHex(output, result.offset);
    output << " size=" << result.size;
    if (result.magic != 0U) {
        output << " magic=";
        WriteHex(output, result.magic);
    }
    if (result.kv3Version != 0U) {
        output << " kv3-version=" << result.kv3Version;
    }
    output << '\n';

    if (!result.preview.empty()) {
        output << prefix << "preview=";
        for (const auto byte : result.preview) {
            output << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<unsigned int>(byte) << ' ';
        }
        output << std::dec << std::setfill(' ') << '\n';
    }
    if (!result.message.empty()) {
        output << prefix << "data-message=" << result.message << '\n';
    }
}

} // namespace

void TextReportWriter::Write(
    const InspectionReport& report,
    std::ostream& output) const {
    const auto& document = report.document;
    const auto& header = document.header;

    output << "VMSourceCONV inspection\n"
           << "=======================\n"
           << "File: " << document.file.path.string() << '\n'
           << "Actual size: " << document.file.bytes.size() << " bytes\n"
           << "Declared size: " << header.declaredFileSize << " bytes\n"
           << "Header version: " << header.headerVersion << '\n'
           << "Resource version: " << header.resourceVersion << '\n'
           << "Block count: " << header.blockCount << '\n'
           << "Block directory: ";
    WriteHex(output, header.blockDirectoryOffset);
    output << "\n\nBlocks\n------\n";

    for (const auto& block : document.blocks) {
        output << '[' << block.index << "] " << block.type.ToString()
               << " offset=";
        WriteHex(output, block.offset);
        output << " size=" << block.size << " bytes entry=";
        WriteHex(output, block.directoryEntryOffset);
        output << '\n';
    }

    if (report.dataInspection.present) {
        output << "\nRoot DATA inspection\n--------------------\n";
        WriteDataSummary(output, report.dataInspection, "");
    }

    output << "\nExternal references\n-------------------\n";
    if (report.externalReferences.empty()) {
        output << "(none decoded)\n";
    } else {
        for (const auto& reference : report.externalReferences) {
            output << "0x" << std::uppercase << std::hex
                   << std::setw(16) << std::setfill('0')
                   << reference.id << std::dec << std::nouppercase
                   << std::setfill(' ') << "  " << reference.name << '\n';
        }
    }

    if (report.resourceGraph.enabled) {
        const auto& resourceGraph = report.resourceGraph;
        const auto& statistics = resourceGraph.statistics;
        output << "\nResource graph\n--------------\n"
               << "Mode: "
               << (resourceGraph.includeAssets
                       ? "all referenced resources"
                       : "structural map resources")
               << '\n'
               << "Maximum depth: " << resourceGraph.maximumDepth << '\n'
               << "Maximum resources: " << resourceGraph.maximumResources << '\n'
               << "Search roots:\n";
        for (const auto& root : resourceGraph.searchRoots) {
            output << "  " << root.string() << '\n';
        }
        output << "Mounted VPKs:\n";
        if (resourceGraph.mountedVpks.empty()) {
            output << "  (none)\n";
        } else {
            for (const auto& path : resourceGraph.mountedVpks) {
                output << "  " << path.string() << '\n';
            }
        }

        output << "\nResolved resources:\n";
        if (resourceGraph.nodes.empty()) {
            output << "(none)\n";
        } else {
            for (const auto& node : resourceGraph.nodes) {
                output << '[' << node.index << "] depth=" << node.depth
                       << " status=" << graph::ToString(node.status)
                       << " source=" << graph::ToString(node.source)
                       << ' ' << node.logicalName;
                if (node.source == graph::ResourceNodeSource::LooseFile) {
                    output << "\n    file=" << node.resolvedPath.string();
                } else if (node.source == graph::ResourceNodeSource::VpkArchive) {
                    output << "\n    archive=" << node.resolvedPath.string()
                           << "\n    entry=" << node.vpkEntryPath;
                }
                if (node.status == graph::ResourceNodeStatus::Loaded) {
                    output << "\n    size=" << node.actualSize
                           << " blocks=" << node.blocks.size()
                           << " references=" << node.externalReferenceCount
                           << '\n';
                    WriteDataSummary(output, node.dataInspection, "    ");
                }
                if (node.parentIndex.has_value()) {
                    output << "    parent=" << *node.parentIndex << '\n';
                }
                if (!node.message.empty()) {
                    output << "    message=" << node.message << '\n';
                }
            }
        }

        output << "\nGraph summary:\n"
               << "  references seen: " << statistics.referencesSeen << '\n'
               << "  references skipped: " << statistics.referencesSkipped << '\n'
               << "  duplicate references: " << statistics.duplicateReferences << '\n'
               << "  depth-limited references: " << statistics.depthLimitedReferences << '\n'
               << "  resources loaded: " << statistics.resourcesLoaded << '\n'
               << "  loaded from loose files: " << statistics.resourcesLoadedLoose << '\n'
               << "  loaded from VPKs: " << statistics.resourcesLoadedFromVpk << '\n'
               << "  resources missing: " << statistics.resourcesMissing << '\n'
               << "  resources failed: " << statistics.resourcesFailed << '\n'
               << "  resource limit reached: "
               << (statistics.resourceLimitReached ? "yes" : "no") << '\n';
    }

    output << "\nDiagnostics\n-----------\n";
    if (report.diagnostics.empty()) {
        output << "No diagnostics.\n";
    } else {
        for (const auto& diagnostic : report.diagnostics) {
            output << core::ToString(diagnostic.severity)
                   << " [" << diagnostic.code << "] ";
            if (diagnostic.offset.has_value()) {
                WriteHex(output, *diagnostic.offset);
                output << ": ";
            }
            output << diagnostic.message << '\n';
        }
    }

    output << "\nSummary: " << report.WarningCount() << " warning(s), "
           << report.ErrorCount() << " error(s)\n";
}

} // namespace vmsourceconv::inspect
