#include "inspect/InspectCommand.h"

#include "core/ParseError.h"
#include "entities/EntityLumpJsonWriter.h"
#include "inspect/BlockDumper.h"
#include "inspect/Inspector.h"
#include "inspect/JsonReportWriter.h"
#include "inspect/TextReportWriter.h"
#include "scene/WorldSceneJsonWriter.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace vmsourceconv::inspect {

core::ExitCode InspectCommand::Execute(const InspectOptions& options) const {
    try {
        const auto report = Inspector{}.Run(options);
        TextReportWriter{}.Write(report, std::cout);

        if (!options.jsonOutput.empty()) {
            JsonReportWriter{}.WriteFile(report, options.jsonOutput);
        }
        if (!options.entitiesJsonOutput.empty()) {
            entities::EntityLumpJsonWriter{}.WriteFile(
                report.entityLumps, options.entitiesJsonOutput);
            std::cout << "Entity JSON: "
                      << options.entitiesJsonOutput.string() << '\n';
        }
        if (!options.sceneJsonOutput.empty()) {
            scene::WorldSceneJsonWriter{}.WriteFile(
                report.worldScene, options.sceneJsonOutput);
            std::cout << "Scene JSON: "
                      << options.sceneJsonOutput.string() << '\n';
        }
        if (!options.dumpDirectory.empty()) {
            BlockDumper{}.Dump(report.document, options.dumpDirectory);
        }

        if (report.ErrorCount() != 0U
            || (options.strict && report.WarningCount() != 0U)) {
            return core::ExitCode::ParseError;
        }
        return core::ExitCode::Success;
    } catch (const core::ParseError& error) {
        std::cerr << "parse error at 0x" << std::hex << error.Offset()
                  << std::dec << ": " << error.what() << '\n';
        return core::ExitCode::ParseError;
    } catch (const std::filesystem::filesystem_error& error) {
        std::cerr << "filesystem error: " << error.what() << '\n';
        return core::ExitCode::OutputError;
    } catch (const std::runtime_error& error) {
        std::cerr << "error: " << error.what() << '\n';
        return core::ExitCode::IoError;
    }
}

} // namespace vmsourceconv::inspect
