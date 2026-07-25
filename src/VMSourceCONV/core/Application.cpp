#include "core/Application.h"

#include "cli/CommandLine.h"
#include "cli/CommandRouter.h"
#include "cli/Usage.h"
#include "core/ExitCode.h"

#include <exception>
#include <iostream>

namespace vmsourceconv::core {

int Application::Run(const int argc, char** argv) const {
    try {
        const auto commandLine = cli::CommandLine::Parse(argc, argv);

        if (!commandLine.error.empty()) {
            std::cerr << "error: " << commandLine.error << "\n\n";
            cli::PrintUsage(std::cerr);
            return static_cast<int>(ExitCode::UsageError);
        }

        if (commandLine.showHelp) {
            cli::PrintUsage(std::cout);
            return static_cast<int>(ExitCode::Success);
        }

        if (commandLine.showVersion) {
            std::cout << "VMSourceCONV 0.2.0\n";
            return static_cast<int>(ExitCode::Success);
        }

        return static_cast<int>(
            cli::CommandRouter{}.Execute(commandLine));
    } catch (const std::exception& exception) {
        std::cerr << "fatal: " << exception.what() << '\n';
        return static_cast<int>(ExitCode::InternalError);
    }
}

} // namespace vmsourceconv::core
