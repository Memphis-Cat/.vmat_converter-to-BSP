#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace vmsourceconv::discovery {

struct ResourceLocation {
    std::string logicalName;
    std::filesystem::path compiledRelativePath;
    std::filesystem::path resolvedPath;
    std::vector<std::filesystem::path> candidates;
    std::string error;

    [[nodiscard]] bool Found() const noexcept {
        return !resolvedPath.empty();
    }
};

class ResourceLocator final {
public:
    ResourceLocator(
        const std::filesystem::path& input,
        const std::vector<std::filesystem::path>& extraRoots);

    [[nodiscard]] ResourceLocation Locate(std::string_view logicalName) const;
    [[nodiscard]] const std::vector<std::filesystem::path>& SearchRoots() const noexcept;

private:
    void AddRoot(const std::filesystem::path& root);

    std::vector<std::filesystem::path> roots_;
};

} // namespace vmsourceconv::discovery
