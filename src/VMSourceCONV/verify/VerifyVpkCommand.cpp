#include "verify/VerifyVpkCommand.h"

#include "verify/VerifyVpkReportWriter.h"
#include "vpk/VpkArchive.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace vmsourceconv::verify {

core::ExitCode VerifyVpkCommand::Execute(
    const VerifyVpkOptions& options) const {
    try {
        const vpk::VpkArchive archive(options.input);
        const auto report = archive.Verify(options.verification);
        VerifyVpkReportWriter{}.WriteText(report, std::cout);
        if (!options.jsonOutput.empty()) {
            VerifyVpkReportWriter{}.WriteJsonFile(report, options.jsonOutput);
            std::cout << "Verification JSON: "
                      << options.jsonOutput.string() << '\n';
        }
        return report.Success()
            ? core::ExitCode::Success
            : core::ExitCode::ParseError;
    } catch (const std::filesystem::filesystem_error& error) {
        std::cerr << "filesystem error: " << error.what() << '\n';
        return core::ExitCode::IoError;
    } catch (const std::runtime_error& error) {
        std::cerr << "VPK verification error: " << error.what() << '\n';
        return core::ExitCode::ParseError;
    }
}

} // namespace vmsourceconv::verify
