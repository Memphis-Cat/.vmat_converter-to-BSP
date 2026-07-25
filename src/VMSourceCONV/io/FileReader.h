#pragma once

#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace vmsourceconv::io {

struct FileData {
    std::filesystem::path path;
    std::vector<std::uint8_t> bytes;
};

class FileReader final {
public:
    static FileData ReadAll(
        const std::filesystem::path& path,
        std::uint64_t maximumBytes =
            static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()));
    static FileData ReadRange(
        const std::filesystem::path& path,
        std::uint64_t offset,
        std::uint64_t length,
        std::uint64_t maximumBytes =
            static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()));
};

} // namespace vmsourceconv::io
