#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vmsourceconv::vpk {

enum class VpkVerificationSeverity {
    Info,
    Warning,
    Error,
};

struct VpkReadOptions {
    bool verifyCrc = true;
    std::uint64_t maximumEntrySize = 1024ULL * 1024ULL * 1024ULL;
};

struct VpkVerificationOptions : VpkReadOptions {
    bool scanOrphanChunks = true;
};

struct VpkVerificationIssue {
    VpkVerificationSeverity severity = VpkVerificationSeverity::Info;
    std::string code;
    std::string message;
};

struct VpkVerificationReport {
    std::filesystem::path directoryFile;
    std::uint32_t version = 0;
    std::size_t directoryEntries = 0;
    std::size_t embeddedEntries = 0;
    std::size_t splitEntries = 0;
    std::size_t duplicatePaths = 0;
    std::size_t referencedChunks = 0;
    std::size_t chunksPresent = 0;
    std::size_t missingChunks = 0;
    std::size_t orphanChunks = 0;
    std::size_t outOfRangeEntries = 0;
    std::size_t crcChecked = 0;
    std::size_t crcPassed = 0;
    std::size_t crcFailed = 0;
    std::uint64_t totalPackageBytes = 0;
    std::vector<VpkVerificationIssue> issues;

    [[nodiscard]] bool Success() const noexcept;
    [[nodiscard]] std::size_t WarningCount() const noexcept;
    [[nodiscard]] std::size_t ErrorCount() const noexcept;
};

[[nodiscard]] const char* ToString(VpkVerificationSeverity severity) noexcept;

} // namespace vmsourceconv::vpk
