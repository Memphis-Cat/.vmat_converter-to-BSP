#include "cli/Usage.h"

#include <ostream>

namespace vmsourceconv::cli {

void PrintUsage(std::ostream& output) {
    output
        << "VMSourceCONV - Source 2 to Source 1 conversion toolkit\n\n"
        << "Usage:\n"
        << "  VMSourceCONV inspect <resource.vmap_c> [options]\n\n"
        << "Inspect options:\n"
        << "  --json <file>            Write a machine-readable inspection report.\n"
        << "  --dump-blocks <dir>      Dump every valid root-resource block.\n"
        << "  --resource-root <dir>    Add a loose Source 2 content search root.\n"
        << "                           May be supplied more than once.\n"
        << "  --follow-references      Recursively inspect structural map resources.\n"
        << "  --include-assets         Also follow models, textures, and materials.\n"
        << "  --max-depth <count>      Dependency depth limit (default: 4).\n"
        << "  --max-resources <count>  Resource count limit (default: 512).\n"
        << "  --no-rerl                Do not decode external resource references.\n"
        << "  --strict                 Return failure when warnings are found.\n"
        << "  -h, --help               Show this help.\n"
        << "  -v, --version            Show the program version.\n\n"
        << "Examples:\n"
        << "  VMSourceCONV inspect maps/de_example.vmap_c --json report.json\n"
        << "  VMSourceCONV inspect maps/de_example.vmap_c "
           "--follow-references --resource-root C:\\cs2\\game\\csgo\n";
}

} // namespace vmsourceconv::cli
