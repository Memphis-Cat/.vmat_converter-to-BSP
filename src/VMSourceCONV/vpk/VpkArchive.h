#pragma once

#include "io/FileReader.h"
#include "vpk/VpkEntry.h"
#include "vpk/VpkVerification.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace vmsourceconv::vpk {

struct VpkV2Sections {
    std::uint32_t fileDataSize = 0;
    std::uint32_t archiveMd5Size = 0;
    std::uint32_t otherMd5Size = 0;
    std::uint32_t signatureSize = 0;
};

class VpkArchive final {
public:
    explicit VpkArchive(const std::filesystem::path& directoryFile);

    [[nodiscard]] const std::filesystem::path& Path() const noexcept;
    [[nodiscard]] std::uint32_t Version() const noexcept;
    [[nodiscard]] std::size_t EntryCount() const noexcept;
    [[nodiscard]] const VpkV2Sections& Sections() const noexcept;
    [[nodiscard]] const std::unordered_map<std::string, VpkEntry>& Entries() const noexcept;
    [[nodiscard]] const std::vector<std::string>& DuplicatePaths() const noexcept;
    [[nodiscard]] const VpkEntry* Find(std::string_view path) const;
    [[nodiscard]] io::FileData Read(
        const VpkEntry& entry,
        const VpkReadOptions& options = {}) const;
    [[nodiscard]] io::FileData ReadRange(
        const VpkEntry& entry,
        std::uint64_t offset,
        std::uint64_t length,
        const VpkReadOptions& options = {}) const;
    [[nodiscard]] VpkVerificationReport Verify(
        const VpkVerificationOptions& options = {}) const;

private:
    void Parse();
    void ValidateV2Sections(std::uint64_t fileSize) const;

    [[nodiscard]] std::filesystem::path ChunkPath(
        std::uint16_t archiveIndex) const;
    [[nodiscard]] std::uint64_t EmbeddedDataOffset() const noexcept;
    [[nodiscard]] std::uint64_t EmbeddedDataLimit(std::uint64_t fileSize) const noexcept;
    static std::string NormalizePath(std::string_view path);

    std::filesystem::path directoryFile_;
    std::uint32_t version_ = 0;
    std::uint32_t headerSize_ = 0;
    std::uint32_t treeSize_ = 0;
    VpkV2Sections sections_;
    std::unordered_map<std::string, VpkEntry> entries_;
    std::vector<std::string> duplicatePaths_;
};

} // namespace vmsourceconv::vpk
