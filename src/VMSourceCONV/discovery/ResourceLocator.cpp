#include "discovery/ResourceLocator.h"

#include "discovery/CompiledResourcePath.h"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace vmsourceconv::discovery {
namespace {

[[nodiscard]] std::filesystem::path MakeAbsolute(const std::filesystem::path& path) {
    std::error_code error;
    auto absolutePath = std::filesystem::absolute(path, error);
    if (error) {
        return path.lexically_normal();
    }
    return absolutePath.lexically_normal();
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

} // namespace

ResourceLocator::ResourceLocator(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& extraRoots) {
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
}

ResourceLocation ResourceLocator::Locate(const std::string_view logicalName) const {
    ResourceLocation location;
    location.logicalName = NormalizeLogicalResourceName(logicalName);

    const auto compiledRelativePath = ToCompiledRelativePath(logicalName);
    if (!compiledRelativePath.has_value()) {
        location.error = "resource path is empty, absolute, or contains parent traversal";
        return location;
    }

    location.compiledRelativePath = *compiledRelativePath;

    for (const auto& root : roots_) {
        const auto candidate = (root / location.compiledRelativePath).lexically_normal();
        location.candidates.push_back(candidate);

        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) {
            location.resolvedPath = candidate;
            return location;
        }
    }

    location.error = "compiled resource was not found in any search root";
    return location;
}

const std::vector<std::filesystem::path>& ResourceLocator::SearchRoots() const noexcept {
    return roots_;
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
