#pragma once

#include "scene/WorldScene.h"
#include "serialization/kv3/Kv3Document.h"

#include <string>

namespace vmsourceconv::scene {

class WorldSceneExtractor final {
public:
    void AddDocument(
        const std::string& logicalName,
        const serialization::kv3::Kv3Document& document,
        WorldScene& scene) const;

    void Finalize(WorldScene& scene) const;

    [[nodiscard]] static bool IsWorldResource(const std::string& logicalName);
    [[nodiscard]] static bool IsWorldNodeResource(const std::string& logicalName);
};

} // namespace vmsourceconv::scene
