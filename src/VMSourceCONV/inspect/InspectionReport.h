#pragma once

#include "core/Diagnostic.h"
#include "resource/ResourceDocument.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::inspect {

struct ExternalReference {
    std::uint64_t id = 0;
    std::string name;
    std::size_t entryOffset = 0;
    std::size_t stringOffset = 0;
};

struct InspectionReport {
    resource::ResourceDocument document;
    std::vector<ExternalReference> externalReferences;
    std::vector<core::Diagnostic> diagnostics;

    [[nodiscard]] std::size_t WarningCount() const noexcept;
    [[nodiscard]] std::size_t ErrorCount() const noexcept;
};

} // namespace vmsourceconv::inspect
