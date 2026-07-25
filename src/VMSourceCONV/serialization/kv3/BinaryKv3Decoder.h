#pragma once

#include "resource/ResourceBlock.h"
#include "resource/ResourceDocument.h"
#include "serialization/kv3/Kv3DecodeLimits.h"
#include "serialization/kv3/Kv3Document.h"

namespace vmsourceconv::serialization::kv3 {

class BinaryKv3Decoder final {
public:
    [[nodiscard]] Kv3Document Decode(
        const resource::ResourceDocument& document,
        const resource::ResourceBlock& dataBlock,
        const Kv3DecodeLimits& limits = {}) const;
};

} // namespace vmsourceconv::serialization::kv3
