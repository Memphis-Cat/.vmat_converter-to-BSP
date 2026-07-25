#pragma once

#include "inspect/data/DataInspectionResult.h"
#include "resource/ResourceDocument.h"

namespace vmsourceconv::inspect::data {

class DataBlockInspector final {
public:
    [[nodiscard]] DataInspectionResult Inspect(
        const resource::ResourceDocument& document) const;
};

} // namespace vmsourceconv::inspect::data
