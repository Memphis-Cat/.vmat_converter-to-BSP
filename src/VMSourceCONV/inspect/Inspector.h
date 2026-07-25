#pragma once

#include "inspect/InspectOptions.h"
#include "inspect/InspectionReport.h"

namespace vmsourceconv::inspect {

class Inspector final {
public:
    InspectionReport Run(const InspectOptions& options) const;
};

} // namespace vmsourceconv::inspect
