#pragma once

#include <cstdint>

namespace vmsourceconv::serialization::kv3 {

enum class Kv3Compression {
    None,
    Lz4,
    Zstd,
    Unknown,
};

[[nodiscard]] Kv3Compression Kv3CompressionFromMethod(std::uint32_t method) noexcept;
[[nodiscard]] const char* ToString(Kv3Compression compression) noexcept;

} // namespace vmsourceconv::serialization::kv3
