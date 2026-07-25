#include "cli/Usage.h"

#include <ostream>

namespace vmsourceconv::cli {

void PrintUsage(std::ostream& output) {
    output
        << "VMSourceCONV - Source 2 to Source 1 conversion toolkit\n\n"
        << "Usage:\n"
        << "  VMSourceCONV inspect <resource.vmap_c> [options]\n\n"
        << "Inspect options:\n"
        << "  --json <file>            Write a machine-readable report.\n"
        << "  --dump-blocks <dir>      Dump every valid root-resource block.\n"
        << "  --inspect-data           Classify DATA blocks and show byte previews.\n"
        << "  --resource-root <dir>    Add a loose content search root.\n"
        << "                           A matching map VPK is selected automatically.\n"
        << "  --vpk <file-or-dir>      Select the primary VPK package or directory.\n"
        << "                           May be supplied more than once.\n"
        << "  --follow-references      Recursively inspect structural resources.\n"
        << "  --include-assets         Also follow models, textures, and materials.\n"
        << "  --max-depth <count>      Dependency depth limit (default: 4).\n"
        << "  --max-resources <count>  Resource count limit (default: 512).\n"
        << "  --log <file>             Copy console output and errors to a file.\n"
        << "  --log=<file>             Same as --log <file>.\n"
        << "  --name.txt               Shorthand for --log name.txt.\n"
        << "  --no-rerl                Do not decode external references.\n"
        << "  --strict                 Return failure when warnings are found.\n"
        << "  -h, --help               Show this help.\n"
        << "  -v, --version            Show the program version.\n\n"
        << "VPK lookup order:\n"
        << "  1. Loose files beside the input and in resource roots.\n"
        << "  2. Explicit --vpk packages and a matching map package.\n"
        << "  3. The recursive ./vpk fallback library, pak01_dir.vpk first.\n\n"
        << "Examples:\n"
        << "  VMSourceCONV inspect de_cache.vmap_c --follow-references "
           "--inspect-data --logs.txt\n"
        << "  VMSourceCONV inspect de_cache.vmap_c --vpk D:\\cache-package "
           "--follow-references --inspect-data --log cache-data.txt\n";
}

} // namespace vmsourceconv::cli
