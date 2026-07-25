#pragma once

#include "core/Diagnostic.h"
#include "data/DataInspectionResult.h"
#include "resource/ResourceDocument.h"

#include <string_view>
#include <vector>

namespace vmsourceconv::data {

class DataBlockInspector final {
public:
    [[nodiscard]] std::vector<DataInspectionResult> Inspect(
        const resource::ResourceDocument& document,
        std::vector<core::Diagnostic>& diagnostics,
        std::string_view context = {}) const;
};

} // namespace vmsourceconv::data
