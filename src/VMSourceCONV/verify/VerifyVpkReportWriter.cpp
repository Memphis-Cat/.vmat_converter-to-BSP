#include "verify/VerifyVpkReportWriter.h"

#include <filesystem>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace vmsourceconv::verify {
namespace {

void WriteJsonString(std::ostream& output, const std::string& value) {
    output << '"';
    for (const unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20U) {
                    output << "?";
                } else {
                    output << static_cast<char>(character);
                }
                break;
        }
    }
    output << '"';
}

} // namespace

void VerifyVpkReportWriter::WriteText(
    const vpk::VpkVerificationReport& report,
    std::ostream& output) const {
    output << "VPK verification\n"
           << "================\n"
           << "Directory file: " << report.directoryFile.string() << '\n'
           << "Version: " << report.version << '\n'
           << "Directory entries: " << report.directoryEntries << '\n'
           << "Embedded entries: " << report.embeddedEntries << '\n'
           << "Split entries: " << report.splitEntries << '\n'
           << "Duplicate paths: " << report.duplicatePaths << '\n'
           << "Referenced chunks: " << report.referencedChunks << '\n'
           << "Chunks present: " << report.chunksPresent << '\n'
           << "Missing chunks: " << report.missingChunks << '\n'
           << "Orphan chunks: " << report.orphanChunks << '\n'
           << "Out-of-range entries: " << report.outOfRangeEntries << '\n'
           << "CRC checked: " << report.crcChecked << '\n'
           << "CRC passed: " << report.crcPassed << '\n'
           << "CRC failed: " << report.crcFailed << '\n'
           << "Total package bytes: " << report.totalPackageBytes << "\n\n"
           << "Issues\n------\n";
    if (report.issues.empty()) {
        output << "No issues.\n";
    } else {
        for (const auto& issue : report.issues) {
            output << vpk::ToString(issue.severity)
                   << " [" << issue.code << "] " << issue.message << '\n';
        }
    }
    output << "\nSummary: " << report.WarningCount() << " warning(s), "
           << report.ErrorCount() << " error(s)\n";
}

void VerifyVpkReportWriter::WriteJsonFile(
    const vpk::VpkVerificationReport& report,
    const std::filesystem::path& path) const {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error(
            "unable to open VPK verification JSON: " + path.string());
    }

    output << "{\n  \"directoryFile\": ";
    WriteJsonString(output, report.directoryFile.generic_string());
    output << ",\n  \"version\": " << report.version
           << ",\n  \"success\": " << (report.Success() ? "true" : "false")
           << ",\n  \"directoryEntries\": " << report.directoryEntries
           << ",\n  \"embeddedEntries\": " << report.embeddedEntries
           << ",\n  \"splitEntries\": " << report.splitEntries
           << ",\n  \"duplicatePaths\": " << report.duplicatePaths
           << ",\n  \"referencedChunks\": " << report.referencedChunks
           << ",\n  \"chunksPresent\": " << report.chunksPresent
           << ",\n  \"missingChunks\": " << report.missingChunks
           << ",\n  \"orphanChunks\": " << report.orphanChunks
           << ",\n  \"outOfRangeEntries\": " << report.outOfRangeEntries
           << ",\n  \"crcChecked\": " << report.crcChecked
           << ",\n  \"crcPassed\": " << report.crcPassed
           << ",\n  \"crcFailed\": " << report.crcFailed
           << ",\n  \"totalPackageBytes\": " << report.totalPackageBytes
           << ",\n  \"warnings\": " << report.WarningCount()
           << ",\n  \"errors\": " << report.ErrorCount()
           << ",\n  \"issues\": [";
    for (std::size_t index = 0; index < report.issues.size(); ++index) {
        const auto& issue = report.issues[index];
        output << (index == 0U ? "\n" : ",\n") << "    {\"severity\": ";
        WriteJsonString(output, vpk::ToString(issue.severity));
        output << ", \"code\": ";
        WriteJsonString(output, issue.code);
        output << ", \"message\": ";
        WriteJsonString(output, issue.message);
        output << '}';
    }
    if (!report.issues.empty()) {
        output << '\n';
    }
    output << "  ]\n}\n";
    if (!output) {
        throw std::runtime_error(
            "unable to write VPK verification JSON: " + path.string());
    }
}

} // namespace vmsourceconv::verify
