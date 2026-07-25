#include "discovery/ResourceLocator.h"

#include "discovery/CompiledResourcePath.h"
#include "discovery/ResourceIdentity.h"
#include "discovery/VpkSearchPolicy.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <system_error>

namespace vmsourceconv::discovery {
namespace {

[[nodiscard]] std::filesystem::path MakeAbsolute(
    const std::filesystem::path& path) {
    std::error_code error;
    const auto absolutePath = std::filesystem::absolute(path, error);
    return (error ? path : absolutePath).lexically_normal();
}

[[nodiscard]] bool SamePath(
    const std::filesystem::path& left,
    const std::filesystem::path& right) {
#ifdef _WIN32
    auto leftString = left.generic_string();
    auto rightString = right.generic_string();
    const auto lowercase = [](std::string& value) {
        std::transform(
            value.begin(), value.end(), value.begin(),
            [](const unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
    };
    lowercase(leftString);
    lowercase(rightString);
    return leftString == rightString;
#else
    return left == right;
#endif
}

void AppendWarnings(
    const char* tier,
    const std::vector<std::string>& source,
    std::vector<std::string>& destination) {
    for (const auto& warning : source) {
        destination.push_back(std::string(tier) + ": " + warning);
    }
}

void AppendUniquePath(
    std::vector<std::filesystem::path>& destination,
    const std::filesystem::path& path) {
    if (std::none_of(
            destination.begin(), destination.end(),
            [&path](const std::filesystem::path& existing) {
                return SamePath(existing, path);
            })) {
        destination.push_back(path);
    }
}

} // namespace

const char* ToString(const ResourceLookupTier tier) noexcept {
    switch (tier) {
        case ResourceLookupTier::None: return "none";
        case ResourceLookupTier::OverrideLoose: return "override-loose";
        case ResourceLookupTier::ExplicitVpk: return "explicit-vpk";
        case ResourceLookupTier::MapVpk: return "map-vpk";
        case ResourceLookupTier::LooseRoot: return "loose-root";
        case ResourceLookupTier::FallbackVpk: return "fallback-vpk";
    }
    return "unknown";
}

ResourceLocator::ResourceLocator(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& extraRoots,
    const std::vector<std::filesystem::path>& explicitVpks,
    const std::vector<std::filesystem::path>& overrideRoots,
    const vpk::VpkReadOptions& readOptions,
    const io::ResourceReadLimits& limits)
    : readOptions_(readOptions), limits_(limits) {
    readOptions_.maximumEntrySize = std::min(
        readOptions_.maximumEntrySize, limits_.maximumVpkEntryBytes);

    for (const auto& root : overrideRoots) {
        AddRoot(overrideRoots_, root);
    }
    for (const auto& root : extraRoots) {
        AddRoot(roots_, root);
    }
    AddRoot(roots_, InferContentRoot(input));
    AddRoot(roots_, input.parent_path());

    std::error_code error;
    const auto currentDirectory = std::filesystem::current_path(error);
    if (!error) {
        AddRoot(roots_, currentDirectory);
    }

    for (const auto& path : explicitVpks) {
        std::error_code pathError;
        if (std::filesystem::is_directory(path, pathError) && !pathError) {
            explicitVpks_.Discover(path);
        } else {
            explicitVpks_.Mount(path);
        }
    }

    std::vector<std::filesystem::path> primaryRoots;
    primaryRoots.push_back(input.parent_path());
    primaryRoots.push_back(InferContentRoot(input));
    primaryRoots.insert(primaryRoots.end(), roots_.begin(), roots_.end());
    for (const auto& candidate : PrimaryPackageCandidates(input, primaryRoots)) {
        std::error_code candidateError;
        if (std::filesystem::is_regular_file(candidate, candidateError)
            && !candidateError) {
            mapVpks_.Mount(candidate);
        }
    }

    for (const auto& directory : FallbackVpkDirectories(input, roots_)) {
        fallbackVpks_.Discover(directory);
    }

    AppendWarnings("explicit VPK", explicitVpks_.Warnings(), vpkWarnings_);
    AppendWarnings("map VPK", mapVpks_.Warnings(), vpkWarnings_);
    AppendWarnings("fallback VPK", fallbackVpks_.Warnings(), vpkWarnings_);
}

ResourceLocation ResourceLocator::Locate(
    const std::string_view logicalName) const {
    ResourceLocation location;
    std::string identityError;
    const auto identity = ResourceIdentity::TryCreate(logicalName, &identityError);
    if (!identity.has_value()) {
        location.error = identityError;
        return location;
    }
    location.logicalName = identity->logicalName;
    location.compiledRelativePath = identity->compiledRelativePath;
    const auto entryPath = identity->key;

    if (LocateLoose(
            overrideRoots_, ResourceLookupTier::OverrideLoose, location)) {
        AppendShadowedVpks(entryPath, {}, location);
        return location;
    }
    if (const auto match = explicitVpks_.Find(entryPath)) {
        location.vpkMatch = *match;
        location.resolvedPath = match->archivePath;
        location.source = ResourceSource::VpkArchive;
        location.tier = ResourceLookupTier::ExplicitVpk;
        AppendShadowedVpks(entryPath, match->archivePath, location);
        return location;
    }
    if (const auto match = mapVpks_.Find(entryPath)) {
        location.vpkMatch = *match;
        location.resolvedPath = match->archivePath;
        location.source = ResourceSource::VpkArchive;
        location.tier = ResourceLookupTier::MapVpk;
        AppendShadowedVpks(entryPath, match->archivePath, location);
        return location;
    }
    if (LocateLoose(roots_, ResourceLookupTier::LooseRoot, location)) {
        AppendShadowedVpks(entryPath, {}, location);
        return location;
    }
    if (const auto match = fallbackVpks_.Find(entryPath)) {
        location.vpkMatch = *match;
        location.resolvedPath = match->archivePath;
        location.source = ResourceSource::VpkArchive;
        location.tier = ResourceLookupTier::FallbackVpk;
        AppendShadowedVpks(entryPath, match->archivePath, location);
        return location;
    }

    location.error =
        "compiled resource was not found in override roots, explicit packages, "
        "the selected map package, loose roots, or the VPK fallback library";
    return location;
}

io::FileData ResourceLocator::Read(
    const ResourceLocation& location) const {
    if (location.source == ResourceSource::LooseFile) {
        return io::FileReader::ReadAll(
            location.resolvedPath, limits_.maximumInputBytes);
    }
    if (location.source == ResourceSource::VpkArchive) {
        switch (location.tier) {
            case ResourceLookupTier::ExplicitVpk:
                return explicitVpks_.Read(location.vpkMatch, readOptions_);
            case ResourceLookupTier::MapVpk:
                return mapVpks_.Read(location.vpkMatch, readOptions_);
            case ResourceLookupTier::FallbackVpk:
                return fallbackVpks_.Read(location.vpkMatch, readOptions_);
            default:
                break;
        }
    }
    throw std::runtime_error("cannot read an unresolved resource");
}

io::FileData ResourceLocator::ReadRange(
    const ResourceLocation& location,
    const std::uint64_t offset,
    const std::uint64_t length) const {
    if (location.source == ResourceSource::LooseFile) {
        return io::FileReader::ReadRange(
            location.resolvedPath,
            offset,
            length,
            limits_.maximumInputBytes);
    }
    if (location.source == ResourceSource::VpkArchive) {
        switch (location.tier) {
            case ResourceLookupTier::ExplicitVpk:
                return explicitVpks_.ReadRange(
                    location.vpkMatch, offset, length, readOptions_);
            case ResourceLookupTier::MapVpk:
                return mapVpks_.ReadRange(
                    location.vpkMatch, offset, length, readOptions_);
            case ResourceLookupTier::FallbackVpk:
                return fallbackVpks_.ReadRange(
                    location.vpkMatch, offset, length, readOptions_);
            default:
                break;
        }
    }
    throw std::runtime_error("cannot range-read an unresolved resource");
}

const std::vector<std::filesystem::path>&
ResourceLocator::SearchRoots() const noexcept { return roots_; }

const std::vector<std::filesystem::path>&
ResourceLocator::OverrideRoots() const noexcept { return overrideRoots_; }

std::vector<std::filesystem::path> ResourceLocator::MountedVpks() const {
    auto paths = explicitVpks_.MountedPaths();
    const auto mapPaths = mapVpks_.MountedPaths();
    const auto fallbackPaths = fallbackVpks_.MountedPaths();
    paths.insert(paths.end(), mapPaths.begin(), mapPaths.end());
    paths.insert(paths.end(), fallbackPaths.begin(), fallbackPaths.end());
    return paths;
}

const std::vector<std::string>&
ResourceLocator::VpkWarnings() const noexcept { return vpkWarnings_; }

void ResourceLocator::AddRoot(
    std::vector<std::filesystem::path>& destination,
    const std::filesystem::path& root) {
    if (root.empty()) {
        return;
    }
    const auto normalized = MakeAbsolute(root);
    if (std::none_of(
            destination.begin(), destination.end(),
            [&normalized](const std::filesystem::path& existing) {
                return SamePath(existing, normalized);
            })) {
        destination.push_back(normalized);
    }
}

bool ResourceLocator::LocateLoose(
    const std::vector<std::filesystem::path>& roots,
    const ResourceLookupTier tier,
    ResourceLocation& location) const {
    for (const auto& root : roots) {
        const auto candidate =
            (root / location.compiledRelativePath).lexically_normal();
        location.candidates.push_back(candidate);
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) {
            location.resolvedPath = candidate;
            location.source = ResourceSource::LooseFile;
            location.tier = tier;
            return true;
        }
    }
    return false;
}

void ResourceLocator::AppendShadowedVpks(
    const std::string& entryPath,
    const std::filesystem::path& selected,
    ResourceLocation& location) const {
    const auto appendRepository = [&](const vpk::VpkRepository& repository) {
        for (const auto& match : repository.FindAll(entryPath)) {
            if (selected.empty() || !SamePath(match.archivePath, selected)) {
                AppendUniquePath(location.shadowedVpks, match.archivePath);
            }
        }
    };
    appendRepository(explicitVpks_);
    appendRepository(mapVpks_);
    appendRepository(fallbackVpks_);
}

} // namespace vmsourceconv::discovery
