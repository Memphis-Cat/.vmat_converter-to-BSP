#include "cli/CommandRouter.h"

#include "inspect/InspectCommand.h"
#include "verify/VerifyVpkCommand.h"

namespace vmsourceconv::cli {

core::ExitCode CommandRouter::Execute(
    const ParsedCommandLine& commandLine) const {
    switch (commandLine.command) {
        case CommandKind::Inspect:
            return inspect::InspectCommand{}.Execute(
                commandLine.inspectOptions);
        case CommandKind::VerifyVpk:
            return verify::VerifyVpkCommand{}.Execute(
                commandLine.verifyVpkOptions);
        case CommandKind::None:
            return core::ExitCode::UsageError;
    }
    return core::ExitCode::InternalError;
}

} // namespace vmsourceconv::cli
