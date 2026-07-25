#pragma once

#include "core/Diagnostic.h"
#include "io/FileReader.h"
#include "resource/ResourceDocument.h"

#include <vector>

namespace vmsourceconv::resource {

class ResourceParser final {
public:
    ResourceDocument Parse(io::FileData file, std::vector<core::Diagnostic>& diagnostics) const;
};

} // namespace vmsourceconv::resource
