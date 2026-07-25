#pragma once

#include "data/DataInspectionResult.h"
#include "resource/ResourceBlock.h"

#include <cstdint>
#include <vector>

namespace vmsourceconv::data {

class DataFormatDetector final {
public:
    [[nodiscard]] DataInspectionResult Inspect(
        const std::vector<std::uint8_t>& bytes,
        const resource::ResourceBlock& block,
        bool hasIntrospectionBlock) const;
};

} // namespace vmsourceconv::data
