#pragma once

#include "inspect/InspectOptions.h"

#include <string>

namespace vmsourceconv::cli {

enum class CommandKind {
    None,
    Inspect,
};

struct ParsedCommandLine {
    CommandKind command = CommandKind::None;
    inspect::InspectOptions inspectOptions;
    bool showHelp = false;
    bool showVersion = false;
    std::string error;
};

class CommandLine final {
public:
    static ParsedCommandLine Parse(int argc, char** argv);
};

} // namespace vmsourceconv::cli
