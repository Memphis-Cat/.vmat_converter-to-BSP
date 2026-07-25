#include "discovery/ResourceLocator.h"

#include "discovery/CompiledResourcePath.h"
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
            value.begin(),
            value.end(),
            value.begin(),
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

} // namespace

ResourceLocator::ResourceLocator(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& extraRoots,
    const std::vector<std::filesystem::path>& explicitVpks) {
    for (const auto& root : extraRoots) {
        AddRoot(root);
    }

    AddRoot(InferContentRoot(input));
    AddRoot(input.parent_path());

    std::error_code error;
    const auto currentDirectory = std::filesystem::current_path(error);
    if (!error) {
        AddRoot(currentDirectory);
    }

    // Explicit --vpk arguments form the highest-priority package tier.
    for (const auto& path : explicitVpks) {
        std::error_code pathError;
        if (std::filesystem::is_directory(path, pathError) && !pathError) {
            primaryVpks_.Discover(path);
        } else {
            primaryVpks_.Mount(path);
        }
    }

    // For a selected de_cache.vmap_c, prefer de_cache.vpk or
    // de_cache_dir.vpk beside the map or in a supplied resource root.
    std::vector<std::filesystem::path> primaryRoots;
    primaryRoots.push_back(input.parent_path());
    primaryRoots.push_back(InferContentRoot(input));
    primaryRoots.insert(primaryRoots.end(), roots_.begin(), roots_.end());
    for (const auto& candidate : PrimaryPackageCandidates(input, primaryRoots)) {
        std::error_code candidateError;
        if (std::filesystem::is_regular_file(candidate, candidateError)
            && !candidateError) {
            primaryVpks_.Mount(candidate);
        }
    }

    // The shared vpk folder is a fallback only. It is searched after loose
    // resources and the selected package fail to provide an exact path.
    for (const auto& directory : FallbackVpkDirectories(input, roots_)) {
        fallbackVpks_.Discover(directory);
    }

    AppendWarnings("primary VPK", primaryVpks_.Warnings(), vpkWarnings_);
    AppendWarnings("fallback VPK", fallbackVpks_.Warnings(), vpkWarnings_);
}

ResourceLocation ResourceLocator::Locate(
    const std::string_view logicalName) const {
    ResourceLocation location;
    location.logicalName = NormalizeLogicalResourceName(logicalName);

    const auto compiledRelativePath = ToCompiledRelativePath(logicalName);
    if (!compiledRelativePath.has_value()) {
        location.error =
            "resource path is empty, absolute, or contains parent traversal";
        return location;
    }

    location.compiledRelativePath = *compiledRelativePath;

    for (const auto& root : roots_) {
        const auto candidate =
            (root / location.compiledRelativePath).lexically_normal();
        location.candidates.push_back(candidate);

        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) {
            location.resolvedPath = candidate;
            location.source = ResourceSource::LooseFile;
            return location;
        }
    }

    const auto entryPath = location.compiledRelativePath.generic_string();

    // Exact full internal path; first primary archive match wins.
    if (const auto match = primaryVpks_.Find(entryPath)) {
        location.vpkMatch = *match;
        location.resolvedPath =
            primaryVpks_.MountedPaths().at(match->archiveIndex);
        location.source = ResourceSource::VpkArchive;
        return location;
    }

    // Only missing primary resources reach the shared fallback library.
    if (const auto match = fallbackVpks_.Find(entryPath)) {
        location.vpkMatch = *match;
        location.resolvedPath =
            fallbackVpks_.MountedPaths().at(match->archiveIndex);
        location.source = ResourceSource::VpkArchive;
        location.fromFallbackVpk = true;
        return location;
    }

    location.error =
        "compiled resource was not found in loose roots, the selected package, "
        "or the vpk fallback library";
    return location;
}

io::FileData ResourceLocator::Read(
    const ResourceLocation& location) const {
    switch (location.source) {
        case ResourceSource::LooseFile:
            return io::FileReader::ReadAll(location.resolvedPath);
        case ResourceSource::VpkArchive:
            return location.fromFallbackVpk
                ? fallbackVpks_.Read(location.vpkMatch)
                : primaryVpks_.Read(location.vpkMatch);
        case ResourceSource::Missing:
            break;
    }

    throw std::runtime_error("cannot read an unresolved resource");
}

const std::vector<std::filesystem::path>&
ResourceLocator::SearchRoots() const noexcept {
    return roots_;
}

std::vector<std::filesystem::path> ResourceLocator::MountedVpks() const {
    auto paths = primaryVpks_.MountedPaths();
    const auto fallbackPaths = fallbackVpks_.MountedPaths();
    paths.insert(paths.end(), fallbackPaths.begin(), fallbackPaths.end());
    return paths;
}

const std::vector<std::string>&
ResourceLocator::VpkWarnings() const noexcept {
    return vpkWarnings_;
}

void ResourceLocator::AddRoot(const std::filesystem::path& root) {
    if (root.empty()) {
        return;
    }

    const auto normalized = MakeAbsolute(root);
    const auto duplicate = std::find_if(
        roots_.begin(),
        roots_.end(),
        [&normalized](const std::filesystem::path& existing) {
            return SamePath(existing, normalized);
        });

    if (duplicate == roots_.end()) {
        roots_.push_back(normalized);
    }
}

} // namespace vmsourceconv::discovery
