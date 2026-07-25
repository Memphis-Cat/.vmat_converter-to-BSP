#include "scene/WorldScene.h"

namespace vmsourceconv::scene {

const char* ToString(const SceneObjectKind kind) noexcept {
    switch (kind) {
        case SceneObjectKind::Model: return "model";
        case SceneObjectKind::Mesh: return "mesh";
        case SceneObjectKind::Aggregate: return "aggregate";
        case SceneObjectKind::Clutter: return "clutter";
    }
    return "unknown";
}

} // namespace vmsourceconv::scene
