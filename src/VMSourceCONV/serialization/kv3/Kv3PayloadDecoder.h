#pragma once

#include "serialization/kv3/BinaryKv3Header.h"
#include "serialization/kv3/Kv3DecodeLimits.h"
#include "serialization/kv3/Kv3Payload.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

class Kv3PayloadDecoder final {
public:
    [[nodiscard]] Kv3Payload Decode(
        const std::vector<std::uint8_t>& resourceBytes,
        std::size_t dataOffset,
        std::size_t dataSize,
        const BinaryKv3Header& header,
        const Kv3DecodeLimits& limits = {}) const;
};

} // namespace vmsourceconv::serialization::kv3
