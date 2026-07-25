#pragma once

#include "core/Diagnostic.h"
#include "resource/ResourceDocument.h"

#include <vector>

namespace vmsourceconv::resource {

class ResourceValidator final {
public:
    void Validate(const ResourceDocument& document, std::vector<core::Diagnostic>& diagnostics) const;
};

} // namespace vmsourceconv::resource
