#pragma once

#include "io/FileReader.h"
#include "io/ResourceReadLimits.h"
#include "vpk/VpkRepository.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vmsourceconv::discovery {

enum class ResourceSource { Missing, LooseFile, VpkArchive };
enum class ResourceLookupTier {
    None,
    OverrideLoose,
    ExplicitVpk,
    MapVpk,
    LooseRoot,
    FallbackVpk,
};

[[nodiscard]] const char* ToString(ResourceLookupTier tier) noexcept;

struct ResourceLocation {
    std::string logicalName;
    std::filesystem::path compiledRelativePath;
    std::filesystem::path resolvedPath;
    std::vector<std::filesystem::path> candidates;
    std::vector<std::filesystem::path> shadowedVpks;
    std::uint64_t sizeHint = 0;
    ResourceSource source = ResourceSource::Missing;
    ResourceLookupTier tier = ResourceLookupTier::None;
    vpk::VpkMatch vpkMatch;
    std::string error;

    [[nodiscard]] bool Found() const noexcept {
        return source != ResourceSource::Missing;
    }
};

class IResourceProvider {
public:
    virtual ~IResourceProvider() = default;
    [[nodiscard]] virtual ResourceLocation Locate(
        std::string_view logicalName) const = 0;
    [[nodiscard]] virtual io::FileData Read(
        const ResourceLocation& location) const = 0;
    [[nodiscard]] virtual io::FileData ReadRange(
        const ResourceLocation& location,
        std::uint64_t offset,
        std::uint64_t length) const = 0;
    [[nodiscard]] virtual const std::vector<std::filesystem::path>&
        SearchRoots() const noexcept = 0;
    [[nodiscard]] virtual const std::vector<std::filesystem::path>&
        OverrideRoots() const noexcept = 0;
    [[nodiscard]] virtual std::vector<std::filesystem::path>
        MountedVpks() const = 0;
    [[nodiscard]] virtual const std::vector<std::string>&
        VpkWarnings() const noexcept = 0;
};

} // namespace vmsourceconv::discovery
