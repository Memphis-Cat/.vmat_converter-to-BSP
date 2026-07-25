#pragma once

#include "io/FileReader.h"
#include "vpk/VpkArchive.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vmsourceconv::vpk {

struct VpkMatch {
    std::size_t archiveIndex = 0;
    std::filesystem::path archivePath;
    std::string entryPath;
};

class VpkRepository final {
public:
    void Mount(const std::filesystem::path& path);
    void Discover(const std::filesystem::path& root);

    [[nodiscard]] std::optional<VpkMatch> Find(
        std::string_view path) const;
    [[nodiscard]] std::vector<VpkMatch> FindAll(
        std::string_view path) const;
    [[nodiscard]] std::uint64_t EntrySize(const VpkMatch& match) const;
    [[nodiscard]] io::FileData Read(
        const VpkMatch& match,
        const VpkReadOptions& options = {}) const;
    [[nodiscard]] io::FileData ReadRange(
        const VpkMatch& match,
        std::uint64_t offset,
        std::uint64_t length,
        const VpkReadOptions& options = {}) const;
    [[nodiscard]] std::vector<VpkVerificationReport> VerifyAll(
        const VpkVerificationOptions& options = {}) const;
    [[nodiscard]] std::vector<std::filesystem::path> MountedPaths() const;
    [[nodiscard]] const std::vector<VpkArchive>& Archives() const noexcept;
    [[nodiscard]] const std::vector<std::string>& Warnings() const noexcept;

private:
    [[nodiscard]] bool AlreadyMounted(
        const std::filesystem::path& path) const;
    bool TryMount(const std::filesystem::path& path);

    std::vector<VpkArchive> archives_;
    std::vector<std::string> warnings_;
};

} // namespace vmsourceconv::vpk
