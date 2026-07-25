#pragma once

#include "resource/ResourceDocument.h"

#include <filesystem>

namespace vmsourceconv::inspect {

class BlockDumper final {
public:
    void Dump(const resource::ResourceDocument& document, const std::filesystem::path& directory) const;
};

} // namespace vmsourceconv::inspect
