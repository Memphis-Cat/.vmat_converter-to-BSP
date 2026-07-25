#pragma once

#include "core/ExitCode.h"
#include "inspect/InspectOptions.h"

namespace vmsourceconv::inspect {

class InspectCommand final {
public:
    core::ExitCode Execute(const InspectOptions& options) const;
};

} // namespace vmsourceconv::inspect
