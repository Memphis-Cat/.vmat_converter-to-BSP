#include "vpk/VpkRepository.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace vmsourceconv::vpk {
namespace {

std::string Lowercase(std::string value) {
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

bool EndsWithIgnoreCase(
    const std::string& value,
    const std::string& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(
        suffix.rbegin(), suffix.rend(), value.rbegin(),
        [](const unsigned char left, const unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        });
}

bool IsVpkCandidate(const std::filesystem::path& path) {
    return EndsWithIgnoreCase(path.filename().string(), ".vpk");
}

int DiscoveryPriority(const std::filesystem::path& path) {
    const auto name = Lowercase(path.filename().string());
    if (name == "pak01_dir.vpk" || name == "pak1_dir.vpk") {
        return 0;
    }
    if (name.size() >= 8U
        && name.rfind("pak", 0) == 0
        && EndsWithIgnoreCase(name, "_dir.vpk")) {
        return 1;
    }
    if (EndsWithIgnoreCase(name, "_dir.vpk")) {
        return 2;
    }
    return 3;
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
    if (!AlreadyMounted(normalized)) {
        archives_.emplace_back(normalized);
    }
}

void VpkRepository::Discover(const std::filesystem::path& root) {
    std::error_code error;
    if (std::filesystem::is_regular_file(root, error) && !error) {
        if (IsVpkCandidate(root)) {
            (void)TryMount(root);
        }
        return;
    }
    error.clear();
    if (!std::filesystem::is_directory(root, error) || error) {
        return;
    }

    std::vector<std::filesystem::path> candidates;
    std::filesystem::recursive_directory_iterator iterator(
        root,
        std::filesystem::directory_options::skip_permission_denied,
        error);
    const std::filesystem::recursive_directory_iterator end;
    while (iterator != end) {
        if (error) {
            error.clear();
            iterator.increment(error);
            continue;
        }
        std::error_code entryError;
        if (iterator->is_regular_file(entryError)
            && !entryError
            && IsVpkCandidate(iterator->path())) {
            candidates.push_back(iterator->path());
        }
        iterator.increment(error);
    }

    std::sort(
        candidates.begin(), candidates.end(),
        [](const std::filesystem::path& left,
           const std::filesystem::path& right) {
            const auto leftPriority = DiscoveryPriority(left);
            const auto rightPriority = DiscoveryPriority(right);
            return leftPriority != rightPriority
                ? leftPriority < rightPriority
                : Lowercase(left.generic_string())
                    < Lowercase(right.generic_string());
        });
    for (const auto& candidate : candidates) {
        (void)TryMount(candidate);
    }
}

std::optional<VpkMatch> VpkRepository::Find(
    const std::string_view path) const {
    const auto matches = FindAll(path);
    return matches.empty()
        ? std::nullopt
        : std::optional<VpkMatch>(matches.front());
}

std::vector<VpkMatch> VpkRepository::FindAll(
    const std::string_view path) const {
    std::vector<VpkMatch> matches;
    for (std::size_t index = 0; index < archives_.size(); ++index) {
        if (const auto* entry = archives_[index].Find(path)) {
            matches.push_back({index, archives_[index].Path(), entry->path});
        }
    }
    return matches;
}

io::FileData VpkRepository::Read(
    const VpkMatch& match,
    const VpkReadOptions& options) const {
    if (match.archiveIndex >= archives_.size()) {
        throw std::runtime_error("invalid VPK repository index");
    }
    const auto* entry = archives_[match.archiveIndex].Find(match.entryPath);
    if (entry == nullptr) {
        throw std::runtime_error("VPK entry disappeared: " + match.entryPath);
    }
    return archives_[match.archiveIndex].Read(*entry, options);
}

io::FileData VpkRepository::ReadRange(
    const VpkMatch& match,
    const std::uint64_t offset,
    const std::uint64_t length,
    const VpkReadOptions& options) const {
    if (match.archiveIndex >= archives_.size()) {
        throw std::runtime_error("invalid VPK repository index");
    }
    const auto* entry = archives_[match.archiveIndex].Find(match.entryPath);
    if (entry == nullptr) {
        throw std::runtime_error("VPK entry disappeared: " + match.entryPath);
    }
    return archives_[match.archiveIndex].ReadRange(
        *entry, offset, length, options);
}
