#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace vmsourceconv::discovery {

[[nodiscard]] std::string InputPackageStem(
    const std::filesystem::path& input);

[[nodiscard]] std::vector<std::filesystem::path> PrimaryPackageCandidates(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& roots);

[[nodiscard]] std::vector<std::filesystem::path> FallbackVpkDirectories(
    const std::filesystem::path& input,
    const std::vector<std::filesystem::path>& roots);

} // namespace vmsourceconv::discovery
