#pragma once

#include "vpk/VpkVerification.h"

#include <filesystem>
#include <iosfwd>

namespace vmsourceconv::verify {

class VerifyVpkReportWriter final {
public:
    void WriteText(
        const vpk::VpkVerificationReport& report,
        std::ostream& output) const;
    void WriteJsonFile(
        const vpk::VpkVerificationReport& report,
        const std::filesystem::path& path) const;
};

} // namespace vmsourceconv::verify
