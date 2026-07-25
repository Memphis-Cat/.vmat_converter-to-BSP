#pragma once

#include "cli/CommandLine.h"
#include "core/ExitCode.h"

namespace vmsourceconv::cli {

class CommandRouter final {
public:
    core::ExitCode Execute(const ParsedCommandLine& commandLine) const;
};

} // namespace vmsourceconv::cli
