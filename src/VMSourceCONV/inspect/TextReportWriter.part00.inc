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

void WriteKv3Summary(
    std::ostream& output,
    const graph::Kv3DecodeSummary& summary,
    const char* prefix) {
    if (!summary.attempted) {
        return;
    }
    output << prefix << "KV3 decode="
           << (summary.succeeded ? "success" : "failed");
    if (summary.succeeded) {
        output << " root=" << summary.rootType
               << " strings=" << summary.stringCount
               << " nodes=" << summary.nodeCount
               << " objects=" << summary.objectCount
               << " arrays=" << summary.arrayCount
               << " blobs=" << summary.binaryBlobCount
               << " max-depth=" << summary.maximumDepth;
    }
    output << '\n';
    if (!summary.message.empty()) {
        output << prefix << "kv3-message=" << summary.message << '\n';
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
        WriteKv3Summary(output, report.rootKv3Decode, "");
        if (!report.rootKv3JsonPath.empty()) {
            output << "KV3 JSON=" << report.rootKv3JsonPath.string() << '\n';
        }
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
