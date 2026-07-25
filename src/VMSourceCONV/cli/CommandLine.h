#pragma once

#include "core/LogOptions.h"
#include "inspect/InspectOptions.h"
#include "verify/VerifyVpkOptions.h"

#include <string>

namespace vmsourceconv::cli {

enum class CommandKind {
    None,
    Inspect,
    VerifyVpk,
};

struct ParsedCommandLine {
    CommandKind command = CommandKind::None;
    inspect::InspectOptions inspectOptions;
    verify::VerifyVpkOptions verifyVpkOptions;
    core::LogOptions logOptions;
    bool showHelp = false;
    bool showVersion = false;
    std::string error;
};

class CommandLine final {
public:
    static ParsedCommandLine Parse(int argc, char** argv);
};

} // namespace vmsourceconv::cli
