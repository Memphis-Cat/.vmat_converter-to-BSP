[[nodiscard]] Kv3Value MakeAggregate(const AggregateRecord& aggregate) {
    auto result = Kv3Value::Object();
    result.AddProperty("sourceIndex", Kv3Value::Unsigned(aggregate.sourceIndex));
    result.AddProperty("sourceResource", Kv3Value::String(aggregate.sourceResource));
    result.AddProperty("worldGroup", Kv3Value::String(aggregate.worldGroup));
    result.AddProperty("renderableModel", Kv3Value::String(aggregate.renderableModel));
    result.AddProperty("layerIndex", Kv3Value::Signed(aggregate.layerIndex));
    result.AddProperty("layerName", Kv3Value::String(aggregate.layerName));
    if (aggregate.hasAllFlags) {
        result.AddProperty("allFlags", Kv3Value::Unsigned(aggregate.allFlags));
    } else if (!aggregate.allFlagsText.empty()) {
        result.AddProperty("allFlagsText", Kv3Value::String(aggregate.allFlagsText));
    }
    if (aggregate.hasAnyFlags) {
        result.AddProperty("anyFlags", Kv3Value::Unsigned(aggregate.anyFlags));
    } else if (!aggregate.anyFlagsText.empty()) {
        result.AddProperty("anyFlagsText", Kv3Value::String(aggregate.anyFlagsText));
    }
    auto fragments = Kv3Value::Array(aggregate.fragments.size());
    for (const auto& fragment : aggregate.fragments) {
        fragments.AddArrayValue(MakeFragment(fragment));
    }
    result.AddProperty("fragments", std::move(fragments));
    result.AddProperty("warnings", MakeStrings(aggregate.warnings));
    return result;
}

[[nodiscard]] Kv3Value MakeWorld(const WorldRecord& world) {
    auto result = Kv3Value::Object();
    result.AddProperty("logicalName", Kv3Value::String(world.logicalName));
    result.AddProperty("worldGroup", Kv3Value::String(world.worldGroup));
    result.AddProperty("entityLumps", MakeStrings(world.entityLumps));
    result.AddProperty("worldNodePrefixes", MakeStrings(world.worldNodePrefixes));
    result.AddProperty(
        "hasWorldLightingInfo",
        Kv3Value::Boolean(world.hasWorldLightingInfo));
    result.AddProperty("warnings", MakeStrings(world.warnings));
    return result;
}

[[nodiscard]] Kv3Value MakeWorldNode(const WorldNodeRecord& node) {
    auto result = Kv3Value::Object();
    result.AddProperty("logicalName", Kv3Value::String(node.logicalName));
    result.AddProperty("worldGroup", Kv3Value::String(node.worldGroup));
    result.AddProperty("layerNames", MakeStrings(node.layerNames));

    auto sceneObjects = Kv3Value::Array(node.sceneObjects.size());
    for (const auto& object : node.sceneObjects) {
        sceneObjects.AddArrayValue(MakeSceneObject(object));
    }
    result.AddProperty("sceneObjects", std::move(sceneObjects));

    auto aggregates = Kv3Value::Array(node.aggregates.size());
    for (const auto& aggregate : node.aggregates) {
        aggregates.AddArrayValue(MakeAggregate(aggregate));
    }
    result.AddProperty("aggregates", std::move(aggregates));

    auto clutter = Kv3Value::Array(node.clutterObjects.size());
    for (const auto& object : node.clutterObjects) {
        clutter.AddArrayValue(MakeSceneObject(object));
    }
    result.AddProperty("clutterObjects", std::move(clutter));
    result.AddProperty("warnings", MakeStrings(node.warnings));
    return result;
}

[[nodiscard]] Kv3Value MakeStatistics(const WorldSceneStatistics& statistics) {
    auto result = Kv3Value::Object();
    result.AddProperty("worlds", Kv3Value::Unsigned(statistics.worldCount));
    result.AddProperty("worldNodes", Kv3Value::Unsigned(statistics.worldNodeCount));
    result.AddProperty("layers", Kv3Value::Unsigned(statistics.layerCount));
    result.AddProperty("sceneObjects", Kv3Value::Unsigned(statistics.sceneObjectCount));
    result.AddProperty("modelInstances", Kv3Value::Unsigned(statistics.modelInstanceCount));
    result.AddProperty("meshInstances", Kv3Value::Unsigned(statistics.meshInstanceCount));
    result.AddProperty("aggregates", Kv3Value::Unsigned(statistics.aggregateCount));
    result.AddProperty(
        "aggregateFragments",
        Kv3Value::Unsigned(statistics.aggregateFragmentCount));
    result.AddProperty("clutterObjects", Kv3Value::Unsigned(statistics.clutterObjectCount));
    result.AddProperty(
        "transformedObjects",
        Kv3Value::Unsigned(statistics.transformedObjectCount));
    result.AddProperty(
        "boundedObjects",
        Kv3Value::Unsigned(statistics.boundedObjectCount));
    result.AddProperty(
        "uniqueRenderables",
        Kv3Value::Unsigned(statistics.uniqueRenderableCount));
    return result;
}

[[nodiscard]] Kv3Document MakeDocument(const WorldScene& scene) {
    Kv3Document document;
    auto root = Kv3Value::Object();
    root.AddProperty("enabled", Kv3Value::Boolean(scene.enabled));
    root.AddProperty("statistics", MakeStatistics(scene.statistics));

    auto worlds = Kv3Value::Array(scene.worlds.size());
    for (const auto& world : scene.worlds) {
        worlds.AddArrayValue(MakeWorld(world));
    }
    root.AddProperty("worlds", std::move(worlds));

    auto nodes = Kv3Value::Array(scene.worldNodes.size());
    for (const auto& node : scene.worldNodes) {
        nodes.AddArrayValue(MakeWorldNode(node));
    }
    root.AddProperty("worldNodes", std::move(nodes));
    root.AddProperty("warnings", MakeStrings(scene.warnings));
    document.root = std::move(root);
    return document;
}

} // namespace

void WorldSceneJsonWriter::Write(
    const WorldScene& scene,
    std::ostream& output) const {
    serialization::kv3::Kv3JsonWriter{}.Write(MakeDocument(scene), output);
}

void WorldSceneJsonWriter::WriteFile(
    const WorldScene& scene,
    const std::filesystem::path& path) const {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error(
            "unable to open world scene JSON file: " + path.string());
    }
    Write(scene, stream);
    if (!stream) {
        throw std::runtime_error(
            "unable to write world scene JSON file: " + path.string());
    }
}

} // namespace vmsourceconv::scene
