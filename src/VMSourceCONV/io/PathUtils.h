#pragma once

#include <filesystem>
#include <string>

namespace vmsourceconv::io {

std::string SafeFileComponent(const std::string& value);
void EnsureDirectory(const std::filesystem::path& directory);

} // namespace vmsourceconv::io
