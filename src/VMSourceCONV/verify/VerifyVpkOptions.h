#pragma once

#include "vpk/VpkVerification.h"

#include <filesystem>

namespace vmsourceconv::verify {

struct VerifyVpkOptions {
    std::filesystem::path input;
    std::filesystem::path jsonOutput;
    vpk::VpkVerificationOptions verification;
};

} // namespace vmsourceconv::verify
