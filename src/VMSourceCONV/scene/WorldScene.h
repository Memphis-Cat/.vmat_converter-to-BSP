#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vmsourceconv::scene {

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Vector4 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 0.0;
};

struct Quaternion {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double w = 1.0;
};

struct Transform {
    bool present = false;
    bool decompositionSucceeded = false;
    std::array<double, 16> matrix{
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    Vector3 origin;
    Vector3 scale{1.0, 1.0, 1.0};
    Quaternion rotation;
};

struct Bounds {
    bool present = false;
    Vector3 minimum;
    Vector3 maximum;
};

enum class SceneObjectKind {
    Model,
    Mesh,
    Aggregate,
    Clutter,
};

[[nodiscard]] const char* ToString(SceneObjectKind kind) noexcept;

struct SceneObjectRecord {
    SceneObjectKind kind = SceneObjectKind::Model;
    std::size_t sourceIndex = 0;
    std::string sourceResource;
    std::string worldGroup;
    std::string renderableModel;
    std::string renderableMesh;
    std::string skin;
    std::int64_t layerIndex = -1;
    std::string layerName;
    Transform transform;
    Bounds bounds;
    bool hasTint = false;
    Vector4 tint;
    bool hasLightingOrigin = false;
    Vector3 lightingOrigin;
    bool hasNumericFlags = false;
    std::uint64_t numericFlags = 0;
    std::string flagsText;
    double fadeStartDistance = 0.0;
    double fadeEndDistance = 0.0;
    std::vector<std::string> warnings;
};

struct AggregateFragmentRecord {
    std::size_t sourceIndex = 0;
    std::int64_t drawCallIndex = -1;
    std::uint64_t lodGroupMask = 0;
    bool hasTransform = false;
    Transform transform;
    Bounds bounds;
    bool hasTint = false;
    Vector4 tint;
    bool hasNumericFlags = false;
    std::uint64_t numericFlags = 0;
    std::string flagsText;
    std::vector<std::string> warnings;
};

struct AggregateRecord {
    std::size_t sourceIndex = 0;
    std::string sourceResource;
    std::string worldGroup;
    std::string renderableModel;
    std::int64_t layerIndex = -1;
    std::string layerName;
    bool hasAllFlags = false;
    std::uint64_t allFlags = 0;
    std::string allFlagsText;
    bool hasAnyFlags = false;
    std::uint64_t anyFlags = 0;
    std::string anyFlagsText;
    std::vector<AggregateFragmentRecord> fragments;
    std::vector<std::string> warnings;
};

struct WorldRecord {
    std::string logicalName;
    std::string worldGroup;
    std::vector<std::string> entityLumps;
    std::vector<std::string> worldNodePrefixes;
    bool hasWorldLightingInfo = false;
    std::vector<std::string> warnings;
};

struct WorldNodeRecord {
    std::string logicalName;
    std::string worldGroup;
    std::vector<std::string> layerNames;
    std::vector<SceneObjectRecord> sceneObjects;
    std::vector<AggregateRecord> aggregates;
    std::vector<SceneObjectRecord> clutterObjects;
    std::vector<std::string> warnings;
};

struct WorldSceneStatistics {
    std::size_t worldCount = 0;
    std::size_t worldNodeCount = 0;
    std::size_t layerCount = 0;
    std::size_t sceneObjectCount = 0;
    std::size_t modelInstanceCount = 0;
    std::size_t meshInstanceCount = 0;
    std::size_t aggregateCount = 0;
    std::size_t aggregateFragmentCount = 0;
    std::size_t clutterObjectCount = 0;
    std::size_t transformedObjectCount = 0;
    std::size_t boundedObjectCount = 0;
    std::size_t uniqueRenderableCount = 0;
};

struct WorldScene {
    bool enabled = false;
    std::vector<WorldRecord> worlds;
    std::vector<WorldNodeRecord> worldNodes;
    WorldSceneStatistics statistics;
    std::vector<std::string> warnings;
};

} // namespace vmsourceconv::scene
