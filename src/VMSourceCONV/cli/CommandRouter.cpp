#include "cli/CommandRouter.h"

#include "inspect/InspectCommand.h"

namespace vmsourceconv::cli {

core::ExitCode CommandRouter::Execute(const ParsedCommandLine& commandLine) const {
    switch (commandLine.command) {
        case CommandKind::Inspect:
            return inspect::InspectCommand{}.Execute(commandLine.inspectOptions);
        case CommandKind::None:
            return core::ExitCode::UsageError;
    }

    return core::ExitCode::InternalError;
}

} // namespace vmsourceconv::cli
