#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace vmsourceconv::discovery {

[[nodiscard]] std::string NormalizeLogicalResourceName(std::string_view value);
[[nodiscard]] std::optional<std::filesystem::path> ToCompiledRelativePath(std::string_view logicalName);
[[nodiscard]] std::filesystem::path InferContentRoot(const std::filesystem::path& input);

} // namespace vmsourceconv::discovery
