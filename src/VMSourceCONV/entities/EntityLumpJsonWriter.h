#pragma once

#include "entities/EntityLumpExport.h"

#include <filesystem>
#include <iosfwd>
#include <vector>

namespace vmsourceconv::entities {

class EntityLumpJsonWriter final {
public:
    void Write(
        const std::vector<EntityLumpExport>& lumps,
        std::ostream& output) const;
    void WriteFile(
        const std::vector<EntityLumpExport>& lumps,
        const std::filesystem::path& path) const;
};

} // namespace vmsourceconv::entities
