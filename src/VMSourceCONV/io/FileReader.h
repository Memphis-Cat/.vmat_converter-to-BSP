#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace vmsourceconv::io {

struct FileData {
    std::filesystem::path path;
    std::vector<std::uint8_t> bytes;
};

class FileReader final {
public:
    static FileData ReadAll(const std::filesystem::path& path);
};

} // namespace vmsourceconv::io
