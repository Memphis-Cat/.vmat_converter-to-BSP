#include "inspect/JsonReportWriter.h"

#include "core/Diagnostic.h"

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
                    output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<unsigned int>(character) << std::dec << std::setfill(' ');
                } else {
                    output << static_cast<char>(character);
                }
                break;
        }
    }
    output << '"';
}

} // namespace

void JsonReportWriter::Write(const InspectionReport& report, std::ostream& output) const {
    const auto& document = report.document;
    const auto& header = document.header;

    output << "{\n  \"file\": ";
    WriteEscaped(output, document.file.path.string());
    output << ",\n  \"actualSize\": " << document.file.bytes.size()
           << ",\n  \"header\": {\n"
           << "    \"declaredFileSize\": " << header.declaredFileSize << ",\n"
           << "    \"headerVersion\": " << header.headerVersion << ",\n"
           << "    \"resourceVersion\": " << header.resourceVersion << ",\n"
           << "    \"blockDirectoryRelativeOffset\": " << header.blockDirectoryRelativeOffset << ",\n"
           << "    \"blockDirectoryOffset\": " << header.blockDirectoryOffset << ",\n"
           << "    \"blockCount\": " << header.blockCount << "\n"
           << "  },\n  \"blocks\": [";

    for (std::size_t index = 0; index < document.blocks.size(); ++index) {
        const auto& block = document.blocks[index];
        output << (index == 0 ? "\n" : ",\n") << "    {\"index\": " << block.index << ", \"type\": ";
        WriteEscaped(output, block.type.ToString());
        output << ", \"directoryEntryOffset\": " << block.directoryEntryOffset
               << ", \"relativeOffset\": " << block.relativeOffset
               << ", \"offset\": " << block.offset
               << ", \"size\": " << block.size << '}';
    }
    output << (document.blocks.empty() ? "]" : "\n  ]");

    output << ",\n  \"externalReferences\": [";
    for (std::size_t index = 0; index < report.externalReferences.size(); ++index) {
        const auto& reference = report.externalReferences[index];
        output << (index == 0 ? "\n" : ",\n") << "    {\"id\": " << reference.id << ", \"name\": ";
        WriteEscaped(output, reference.name);
        output << ", \"entryOffset\": " << reference.entryOffset
               << ", \"stringOffset\": " << reference.stringOffset << '}';
    }
    output << (report.externalReferences.empty() ? "]" : "\n  ]");

    output << ",\n  \"diagnostics\": [";
    for (std::size_t index = 0; index < report.diagnostics.size(); ++index) {
        const auto& diagnostic = report.diagnostics[index];
        output << (index == 0 ? "\n" : ",\n") << "    {\"severity\": ";
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
           << ",\n  \"summary\": {\"warnings\": " << report.WarningCount()
           << ", \"errors\": " << report.ErrorCount() << "}\n}\n";
}

void JsonReportWriter::WriteFile(
    const InspectionReport& report,
    const std::filesystem::path& path) const {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("unable to open JSON output file: " + path.string());
    }

    Write(report, stream);
    if (!stream) {
        throw std::runtime_error("unable to write JSON output file: " + path.string());
    }
}

} // namespace vmsourceconv::inspect
