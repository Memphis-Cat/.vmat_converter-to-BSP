#pragma once

#include "inspect/InspectionReport.h"

#include <iosfwd>

namespace vmsourceconv::inspect {

class TextReportWriter final {
public:
    void Write(const InspectionReport& report, std::ostream& output) const;
};

} // namespace vmsourceconv::inspect
