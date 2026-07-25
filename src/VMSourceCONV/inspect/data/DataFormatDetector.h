#pragma once

#include "inspect/data/DataInspectionResult.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vmsourceconv::inspect::data {

class DataFormatDetector final {
public:
    [[nodiscard]] DataInspectionResult Detect(
        const std::vector<std::uint8_t>& bytes,
        std::size_t offset,
        std::uint32_t size) const;
};

} // namespace vmsourceconv::inspect::data
