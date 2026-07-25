#include "inspect/Inspector.h"

#include "graph/ResourceGraphBuilder.h"
#include "inspect/RerlInspector.h"
#include "io/FileReader.h"
#include "resource/ResourceParser.h"

namespace vmsourceconv::inspect {

InspectionReport Inspector::Run(const InspectOptions& options) const {
    InspectionReport report;
    auto file = io::FileReader::ReadAll(options.input);
    report.document = resource::ResourceParser{}.Parse(
        std::move(file),
        report.diagnostics);

    if (options.inspectExternalReferences) {
        report.externalReferences = RerlInspector{}.Inspect(
            report.document,
            report.diagnostics);
    }

    if (options.followReferences) {
        report.resourceGraph = graph::ResourceGraphBuilder{}.Build(
            options.input,
            report.externalReferences,
            options.resourceRoots,
            options.vpkPaths,
            options.includeAssets,
            options.maximumDepth,
            options.maximumResources,
            report.diagnostics);
    }

    return report;
}

} // namespace vmsourceconv::inspect
