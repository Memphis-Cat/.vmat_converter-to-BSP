#include "core/Application.h"

#include "cli/CommandLine.h"
#include "cli/CommandRouter.h"
#include "cli/Usage.h"
#include "core/ExitCode.h"
#include "core/LogSession.h"

#include <exception>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace vmsourceconv::core {
namespace {

constexpr const char* Version = "0.8.0";

} // namespace

int Application::Run(const int argc, char** argv) const {
#ifdef _WIN32
    (void)SetConsoleOutputCP(CP_UTF8);
    (void)SetConsoleCP(CP_UTF8);
#endif

    try {
        const auto commandLine = cli::CommandLine::Parse(argc, argv);
        LogSession logSession(commandLine.logOptions);
        logSession.WriteHeader(Version, argc, argv);
        if (!logSession.Path().empty()
            && commandLine.logOptions.verbosity != Verbosity::Quiet) {
            std::cout << "Log file: " << logSession.Path().string() << "\n\n";
        }

        int exitCode = static_cast<int>(ExitCode::Success);
        if (!commandLine.error.empty()) {
            std::cerr << "error: " << commandLine.error << "\n\n";
            cli::PrintUsage(std::cerr);
            exitCode = static_cast<int>(ExitCode::UsageError);
        } else if (commandLine.showHelp) {
            cli::PrintUsage(std::cout);
        } else if (commandLine.showVersion) {
            std::cout << "VMSourceCONV " << Version << '\n';
        } else {
            exitCode = static_cast<int>(
                cli::CommandRouter{}.Execute(commandLine));
        }

        logSession.Finish(exitCode);
        return exitCode;
    } catch (const std::exception& exception) {
        std::cerr << "fatal: " << exception.what() << '\n';
        return static_cast<int>(ExitCode::InternalError);
    }
}

} // namespace vmsourceconv::core
