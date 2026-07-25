#pragma once

#include <filesystem>

namespace vmsourceconv::inspect {

struct InspectOptions {
    std::filesystem::path input;
    std::filesystem::path jsonOutput;
    std::filesystem::path dumpDirectory;
    bool inspectExternalReferences = true;
    bool strict = false;
};

} // namespace vmsourceconv::inspect
