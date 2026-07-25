#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::inspect::data {

enum class DataFormat {
    None,
    Empty,
    BinaryKv3,
    LegacyBinaryKv3,
    Unknown,
};

[[nodiscard]] const char* ToString(DataFormat format) noexcept;

struct DataInspectionResult {
    bool present = false;
    std::size_t offset = 0;
    std::uint32_t size = 0;
    DataFormat format = DataFormat::None;
    std::uint32_t magic = 0;
    std::uint32_t kv3Version = 0;
    std::vector<std::uint8_t> preview;
    std::string message;
};

} // namespace vmsourceconv::inspect::data
