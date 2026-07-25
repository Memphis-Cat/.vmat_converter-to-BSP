#include "cli/Usage.h"

#include <ostream>

namespace vmsourceconv::cli {

void PrintUsage(std::ostream& output) {
    output
        << "VMSourceCONV - Source 2 to Source 1 conversion toolkit\n\n"
        << "Usage:\n"
        << "  VMSourceCONV inspect <resource.vmap_c> [options]\n\n"
        << "Inspect options:\n"
        << "  --json <file>         Write a machine-readable inspection report.\n"
        << "  --dump-blocks <dir>   Dump every valid resource block as a binary file.\n"
        << "  --no-rerl             Do not decode external resource references.\n"
        << "  --strict              Return failure when warnings are found.\n"
        << "  -h, --help            Show this help.\n"
        << "  -v, --version         Show the program version.\n\n"
        << "Example:\n"
        << "  VMSourceCONV inspect maps/de_example.vmap_c --json report.json --dump-blocks blocks\n";
}

} // namespace vmsourceconv::cli
