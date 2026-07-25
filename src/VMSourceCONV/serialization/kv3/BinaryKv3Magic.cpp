#include "serialization/kv3/BinaryKv3Magic.h"

namespace vmsourceconv::serialization::kv3 {

bool IsBinaryKv3Magic(const std::uint32_t magic) noexcept {
    return BinaryKv3Version(magic) >= 0;
}

int BinaryKv3Version(const std::uint32_t magic) noexcept {
    switch (magic) {
        case BinaryKv3Magic0: return 0;
        case BinaryKv3Magic1: return 1;
        case BinaryKv3Magic2: return 2;
        case BinaryKv3Magic3: return 3;
        case BinaryKv3Magic4: return 4;
        case BinaryKv3Magic5: return 5;
        default: return -1;
    }
}

const char* BinaryKv3MagicName(const std::uint32_t magic) noexcept {
    switch (magic) {
        case BinaryKv3Magic0: return "VKV3-legacy";
        case BinaryKv3Magic1: return "KV3-v1";
        case BinaryKv3Magic2: return "KV3-v2";
        case BinaryKv3Magic3: return "KV3-v3";
        case BinaryKv3Magic4: return "KV3-v4";
        case BinaryKv3Magic5: return "KV3-v5";
        default: return "unknown";
    }
}

} // namespace vmsourceconv::serialization::kv3
