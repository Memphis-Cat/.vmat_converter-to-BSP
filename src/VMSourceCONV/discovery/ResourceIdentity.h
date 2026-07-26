#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace vmsourceconv::discovery {

struct ResourceIdentity {
    std::string logicalName;
    std::filesystem::path compiledRelativePath;
    std::string key;

    [[nodiscard]] static std::optional<ResourceIdentity> TryCreate(
        std::string_view value,
        std::string* error = nullptr);
};

} // namespace vmsourceconv::discovery
