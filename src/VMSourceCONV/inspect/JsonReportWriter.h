#pragma once

#include "inspect/InspectionReport.h"

#include <filesystem>
#include <iosfwd>

namespace vmsourceconv::inspect {

class JsonReportWriter final {
public:
    void Write(const InspectionReport& report, std::ostream& output) const;
    void WriteFile(const InspectionReport& report, const std::filesystem::path& path) const;
};

} // namespace vmsourceconv::inspect
