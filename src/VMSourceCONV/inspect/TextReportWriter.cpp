#include "inspect/TextReportWriter.h"

#include "core/Diagnostic.h"

#include <iomanip>
#include <ostream>

namespace vmsourceconv::inspect {
namespace {

void WriteHex(std::ostream& output, const std::size_t value) {
    output << "0x" << std::uppercase << std::hex << value << std::dec << std::nouppercase;
}

} // namespace

void TextReportWriter::Write(const InspectionReport& report, std::ostream& output) const {
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
        output << '[' << block.index << "] " << block.type.ToString() << " offset=";
        WriteHex(output, block.offset);
        output << " size=" << block.size << " bytes entry=";
        WriteHex(output, block.directoryEntryOffset);
        output << '\n';
    }

    output << "\nExternal references\n-------------------\n";
    if (report.externalReferences.empty()) {
        output << "(none decoded)\n";
    } else {
        for (const auto& reference : report.externalReferences) {
            output << "0x" << std::uppercase << std::hex << std::setw(16) << std::setfill('0')
                   << reference.id << std::dec << std::nouppercase << std::setfill(' ')
                   << "  " << reference.name << '\n';
        }
    }

    output << "\nDiagnostics\n-----------\n";
    if (report.diagnostics.empty()) {
        output << "No diagnostics.\n";
    } else {
        for (const auto& diagnostic : report.diagnostics) {
            output << core::ToString(diagnostic.severity) << " [" << diagnostic.code << "] ";
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
