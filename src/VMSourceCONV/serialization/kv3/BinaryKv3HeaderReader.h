#pragma once

#include "serialization/kv3/BinaryKv3Header.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

class BinaryKv3HeaderReader final {
public:
    [[nodiscard]] BinaryKv3Header Read(
        const std::vector<std::uint8_t>& bytes,
        std::size_t offset,
        std::size_t size) const;
};

} // namespace vmsourceconv::serialization::kv3
