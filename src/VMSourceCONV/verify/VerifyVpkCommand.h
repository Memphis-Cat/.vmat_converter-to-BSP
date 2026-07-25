#pragma once

#include "core/ExitCode.h"
#include "verify/VerifyVpkOptions.h"

namespace vmsourceconv::verify {

class VerifyVpkCommand final {
public:
    [[nodiscard]] core::ExitCode Execute(
        const VerifyVpkOptions& options) const;
};

} // namespace vmsourceconv::verify
