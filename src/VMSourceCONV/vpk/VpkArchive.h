#pragma once

#include "io/FileReader.h"
#include "vpk/VpkEntry.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace vmsourceconv::vpk {

class VpkArchive final {
public:
    explicit VpkArchive(const std::filesystem::path& directoryFile);

    [[nodiscard]] const std::filesystem::path& Path() const noexcept;
    [[nodiscard]] std::uint32_t Version() const noexcept;
    [[nodiscard]] std::size_t EntryCount() const noexcept;
    [[nodiscard]] const VpkEntry* Find(std::string_view path) const;
    [[nodiscard]] io::FileData Read(const VpkEntry& entry) const;

private:
    void Parse();

    [[nodiscard]] std::filesystem::path ChunkPath(
        std::uint16_t archiveIndex) const;
    static std::string NormalizePath(std::string_view path);

    std::filesystem::path directoryFile_;
    std::uint32_t version_ = 0;
    std::uint32_t headerSize_ = 0;
    std::uint32_t treeSize_ = 0;
    std::unordered_map<std::string, VpkEntry> entries_;
};

} // namespace vmsourceconv::vpk
