#pragma once

#include "core/Diagnostic.h"
#include "inspect/InspectionReport.h"
#include "resource/ResourceDocument.h"

#include <vector>

namespace vmsourceconv::inspect {

class RerlInspector final {
public:
    std::vector<ExternalReference> Inspect(
        const resource::ResourceDocument& document,
        std::vector<core::Diagnostic>& diagnostics) const;
};

} // namespace vmsourceconv::inspect
