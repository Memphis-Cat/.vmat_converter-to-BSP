#pragma once

#include "data/DataFormat.h"
#include "serialization/kv3/BinaryKv3Header.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vmsourceconv::data {

struct DataInspectionResult {
    std::size_t blockIndex = 0;
    std::size_t offset = 0;
    std::uint32_t size = 0;
    DataFormat format = DataFormat::Opaque;
    std::string prefixHex;
    std::optional<serialization::kv3::BinaryKv3Header> binaryKv3;
    std::vector<std::string> notes;
};

} // namespace vmsourceconv::data
