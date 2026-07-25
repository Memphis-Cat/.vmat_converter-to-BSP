#pragma once

#include "serialization/kv3/BinaryKv3Header.h"
#include "serialization/kv3/Kv3Buffer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

struct Kv3Payload {
    BinaryKv3Header header;
    Kv3Buffer mainBuffer;
    Kv3Buffer auxiliaryBuffer;
    std::vector<std::string> strings;
    std::vector<std::uint8_t> typeStream;
    std::vector<std::int32_t> objectLengths;
    std::vector<std::int32_t> binaryBlobLengths;
    std::vector<std::uint8_t> binaryBlobs;
};

} // namespace vmsourceconv::serialization::kv3
