#pragma once

#include "discovery/ResourceProvider.h"

#include <filesystem>
#include <string>
#include <vector>

namespace vmsourceconv::discovery {

class ResourceLocator final : public IResourceProvider {
public:
    ResourceLocator(
        const std::filesystem::path& input,
        const std::vector<std::filesystem::path>& extraRoots,
        const std::vector<std::filesystem::path>& explicitVpks,
        const std::vector<std::filesystem::path>& overrideRoots = {},
        const vpk::VpkReadOptions& readOptions = {},
        const io::ResourceReadLimits& limits = {});

    [[nodiscard]] ResourceLocation Locate(
        std::string_view logicalName) const override;
    [[nodiscard]] io::FileData Read(
        const ResourceLocation& location) const override;
    [[nodiscard]] io::FileData ReadRange(
        const ResourceLocation& location,
        std::uint64_t offset,
        std::uint64_t length) const override;
    [[nodiscard]] const std::vector<std::filesystem::path>&
        SearchRoots() const noexcept override;
    [[nodiscard]] const std::vector<std::filesystem::path>&
        OverrideRoots() const noexcept override;
    [[nodiscard]] std::vector<std::filesystem::path>
        MountedVpks() const override;
    [[nodiscard]] const std::vector<std::string>&
        VpkWarnings() const noexcept override;

private:
    void AddRoot(
        std::vector<std::filesystem::path>& destination,
        const std::filesystem::path& root);
    [[nodiscard]] bool LocateLoose(
        const std::vector<std::filesystem::path>& roots,
        ResourceLookupTier tier,
        ResourceLocation& location) const;
    void AppendShadowedVpks(
        const std::string& entryPath,
        const std::filesystem::path& selected,
        ResourceLocation& location) const;

    std::vector<std::filesystem::path> overrideRoots_;
    std::vector<std::filesystem::path> roots_;
    vpk::VpkRepository explicitVpks_;
    vpk::VpkRepository mapVpks_;
    vpk::VpkRepository fallbackVpks_;
    vpk::VpkReadOptions readOptions_;
    io::ResourceReadLimits limits_;
    std::vector<std::string> vpkWarnings_;
};

} // namespace vmsourceconv::discovery
