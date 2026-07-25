#pragma once

#include "io/FileReader.h"
#include "vpk/VpkRepository.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vmsourceconv::discovery {

enum class ResourceSource {
    Missing,
    LooseFile,
    VpkArchive,
};

struct ResourceLocation {
    std::string logicalName;
    std::filesystem::path compiledRelativePath;
    std::filesystem::path resolvedPath;
    std::vector<std::filesystem::path> candidates;
    ResourceSource source = ResourceSource::Missing;
    vpk::VpkMatch vpkMatch;
    bool fromFallbackVpk = false;
    std::string error;

    [[nodiscard]] bool Found() const noexcept {
        return source != ResourceSource::Missing;
    }
};

class ResourceLocator final {
public:
    ResourceLocator(
        const std::filesystem::path& input,
        const std::vector<std::filesystem::path>& extraRoots,
        const std::vector<std::filesystem::path>& explicitVpks);

    [[nodiscard]] ResourceLocation Locate(std::string_view logicalName) const;
    [[nodiscard]] io::FileData Read(const ResourceLocation& location) const;
    [[nodiscard]] const std::vector<std::filesystem::path>& SearchRoots() const noexcept;
    [[nodiscard]] std::vector<std::filesystem::path> MountedVpks() const;
    [[nodiscard]] const std::vector<std::string>& VpkWarnings() const noexcept;

private:
    void AddRoot(const std::filesystem::path& root);

    std::vector<std::filesystem::path> roots_;
    vpk::VpkRepository primaryVpks_;
    vpk::VpkRepository fallbackVpks_;
    std::vector<std::string> vpkWarnings_;
};

} // namespace vmsourceconv::discovery
