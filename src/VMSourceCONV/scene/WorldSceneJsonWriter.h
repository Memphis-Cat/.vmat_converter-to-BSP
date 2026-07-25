#pragma once

#include "scene/WorldScene.h"

#include <filesystem>
#include <iosfwd>

namespace vmsourceconv::scene {

class WorldSceneJsonWriter final {
public:
    void Write(const WorldScene& scene, std::ostream& output) const;
    void WriteFile(
        const WorldScene& scene,
        const std::filesystem::path& path) const;
};

} // namespace vmsourceconv::scene
