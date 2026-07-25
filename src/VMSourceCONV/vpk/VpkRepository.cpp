#include "vpk/VpkRepository.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace vmsourceconv::vpk {
namespace {

bool EndsWithIgnoreCase(
    const std::string& value,
    const std::string& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }

    return std::equal(
        suffix.rbegin(),
        suffix.rend(),
        value.rbegin(),
        [](const unsigned char left, const unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        });
}

bool HasVpkMagic(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }

    unsigned char bytes[4]{};
    stream.read(reinterpret_cast<char*>(bytes), 4);
    return stream.gcount() == 4
        && bytes[0] == 0x34U
        && bytes[1] == 0x12U
        && bytes[2] == 0xAAU
        && bytes[3] == 0x55U;
}

std::filesystem::path NormalizeAbsolute(
    const std::filesystem::path& path) {
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    return (error ? path : absolute).lexically_normal();
}

} // namespace

void VpkRepository::Mount(const std::filesystem::path& path) {
    const auto normalized = NormalizeAbsolute(path);
    if (AlreadyMounted(normalized)) {
        return;
    }

    archives_.emplace_back(normalized);
}

void VpkRepository::Discover(const std::filesystem::path& root) {
    std::error_code error;

    if (std::filesystem::is_regular_file(root, error) && !error) {
        if (EndsWithIgnoreCase(root.filename().string(), ".vpk")) {
            (void)TryMount(root);
        }
        return;
    }

    error.clear();
    if (!std::filesystem::is_directory(root, error) || error) {
        return;
    }

    std::vector<std::filesystem::path> candidates;
    for (std::filesystem::directory_iterator iterator(root, error), end;
         !error && iterator != end;
         iterator.increment(error)) {
        std::error_code entryError;
        if (!iterator->is_regular_file(entryError) || entryError) {
            continue;
        }

        if (EndsWithIgnoreCase(
                iterator->path().filename().string(),
                ".vpk")) {
            candidates.push_back(iterator->path());
        }
    }

    std::sort(candidates.begin(), candidates.end());
    for (const auto& candidate : candidates) {
        (void)TryMount(candidate);
    }
}

std::optional<VpkMatch> VpkRepository::Find(
    const std::string_view path) const {
    for (std::size_t index = 0; index < archives_.size(); ++index) {
        if (const auto* entry = archives_[index].Find(path)) {
            return VpkMatch{index, entry->path};
        }
    }

    return std::nullopt;
}

io::FileData VpkRepository::Read(const VpkMatch& match) const {
    if (match.archiveIndex >= archives_.size()) {
        throw std::runtime_error("invalid VPK repository index");
    }

    const auto* entry = archives_[match.archiveIndex].Find(match.entryPath);
    if (entry == nullptr) {
        throw std::runtime_error(
            "VPK entry disappeared: " + match.entryPath);
    }

    return archives_[match.archiveIndex].Read(*entry);
}

std::vector<std::filesystem::path> VpkRepository::MountedPaths() const {
    std::vector<std::filesystem::path> paths;
    paths.reserve(archives_.size());

    for (const auto& archive : archives_) {
        paths.push_back(archive.Path());
    }

    return paths;
}

const std::vector<std::string>& VpkRepository::Warnings() const noexcept {
    return warnings_;
}

bool VpkRepository::AlreadyMounted(
    const std::filesystem::path& path) const {
    return std::any_of(
        archives_.begin(),
        archives_.end(),
        [&path](const VpkArchive& archive) {
            return archive.Path() == path;
        });
}

bool VpkRepository::TryMount(const std::filesystem::path& path) {
    const auto normalized = NormalizeAbsolute(path);
    if (AlreadyMounted(normalized)) {
        return true;
    }

    // Split data chunks normally do not contain the VPK directory header.
    // Ignore them silently while scanning a map-package folder.
    if (!HasVpkMagic(normalized)) {
        return false;
    }

    try {
        archives_.emplace_back(normalized);
        return true;
    } catch (const std::exception& exception) {
        warnings_.push_back(exception.what());
        return false;
    }
}

} // namespace vmsourceconv::vpk
