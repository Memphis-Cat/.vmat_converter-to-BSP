#pragma once

#include "serialization/kv3/Kv3Document.h"
#include "serialization/kv3/Kv3Value.h"

#include <cstddef>
#include <string>
#include <vector>

namespace vmsourceconv::entities {

struct EntityLumpExport {
    std::string logicalName;
    std::string lumpName;
    std::vector<std::string> childLumps;
    std::vector<serialization::kv3::Kv3Value> entities;
    std::size_t skippedWithoutClassname = 0;
    std::size_t preservedBinaryEntities = 0;
    std::vector<std::string> warnings;
};

class EntityLumpExtractor final {
public:
    [[nodiscard]] EntityLumpExport Extract(
        std::string logicalName,
        const serialization::kv3::Kv3Document& document) const;
};

} // namespace vmsourceconv::entities
