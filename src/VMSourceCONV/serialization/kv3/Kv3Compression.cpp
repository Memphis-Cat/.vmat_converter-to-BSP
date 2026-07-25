#include "serialization/kv3/Kv3Compression.h"

namespace vmsourceconv::serialization::kv3 {

Kv3Compression Kv3CompressionFromMethod(const std::uint32_t method) noexcept {
    switch (method) {
        case 0U: return Kv3Compression::None;
        case 1U: return Kv3Compression::Lz4;
        case 2U: return Kv3Compression::Zstd;
        default: return Kv3Compression::Unknown;
    }
}

const char* ToString(const Kv3Compression compression) noexcept {
    switch (compression) {
        case Kv3Compression::None: return "none";
        case Kv3Compression::Lz4: return "lz4";
        case Kv3Compression::Zstd: return "zstd";
        case Kv3Compression::Unknown: return "unknown";
    }

    return "unknown";
}

} // namespace vmsourceconv::serialization::kv3
