    AggregateRecord result;
    result.sourceResource = logicalName;
    result.worldGroup = worldGroup;
    result.sourceIndex = sourceIndex;
    if (source.Type() != Kv3ValueType::Object) {
        result.warnings.push_back("aggregate is not an object");
        return result;
    }

    result.renderableModel = StringProperty(source, "m_renderableModel");
    if (const auto* layer = source.Find("m_nLayer")) {
        (void)TrySigned(*layer, result.layerIndex);
    }
    result.layerName = LayerNameAt(layers, result.layerIndex);
    ReadFlags(
        source.Find("m_allFlags"),
        result.hasAllFlags,
        result.allFlags,
        result.allFlagsText);
    ReadFlags(
        source.Find("m_anyFlags"),
        result.hasAnyFlags,
        result.anyFlags,
        result.anyFlagsText);

    const Kv3Value* transforms = source.Find("m_fragmentTransforms");
    const Kv3Value* meshes = source.Find("m_aggregateMeshes");
    if (meshes == nullptr || meshes->Type() != Kv3ValueType::Array) {
        result.warnings.push_back("m_aggregateMeshes is missing or not an array");
        return result;
    }

    const std::vector<Kv3Value>* transformValues = nullptr;
    if (transforms != nullptr && transforms->Type() == Kv3ValueType::Array) {
        transformValues = &transforms->ArrayValues();
    }
    std::size_t transformIndex = 0;
    result.fragments.reserve(meshes->ArrayValues().size());
    for (std::size_t index = 0; index < meshes->ArrayValues().size(); ++index) {
        AggregateFragmentRecord fragment;
        fragment.sourceIndex = index;
        const auto& sourceFragment = meshes->ArrayValues()[index];
        if (sourceFragment.Type() != Kv3ValueType::Object) {
            fragment.warnings.push_back("aggregate fragment is not an object");
            result.fragments.push_back(std::move(fragment));
            continue;
        }

        if (const auto* drawCall = sourceFragment.Find("m_nDrawCallIndex")) {
            (void)TrySigned(*drawCall, fragment.drawCallIndex);
        }
        if (const auto* lod = sourceFragment.Find("m_nLODGroupMask")) {
            (void)TryUnsigned(*lod, fragment.lodGroupMask);
        }
        ReadFlags(
            sourceFragment.Find("m_objectFlags"),
            fragment.hasNumericFlags,
            fragment.numericFlags,
            fragment.flagsText);
        if (const auto* tint = sourceFragment.Find("m_vTintColor");
            tint != nullptr) {
            fragment.hasTint = TryVector4(*tint, fragment.tint);
            if (!fragment.hasTint) {
                fragment.warnings.push_back("m_vTintColor is not a numeric vector");
            }
        }
        if (const auto* bounds = sourceFragment.Find("m_vWorldBounds");
            bounds != nullptr) {
            (void)TryBounds(*bounds, fragment.bounds);
        }

        if (const auto* hasTransform = sourceFragment.Find("m_bHasTransform")) {
            (void)TryBoolean(*hasTransform, fragment.hasTransform);
        }
        if (fragment.hasTransform) {
            if (transformValues == nullptr || transformIndex >= transformValues->size()) {
                fragment.warnings.push_back("fragment transform stream is truncated");
            } else if (!TryTransform(
                           (*transformValues)[transformIndex],
                           fragment.transform)) {
                fragment.warnings.push_back("fragment transform has an unsupported layout");
            }
            ++transformIndex;
        }
        result.fragments.push_back(std::move(fragment));
    }

    if (transformValues != nullptr && transformIndex != transformValues->size()) {
        result.warnings.push_back("fragment transform stream has unused entries");
    }
    if (result.renderableModel.empty()) {
        result.warnings.push_back("aggregate has no renderable model");
    }
    return result;
}

[[nodiscard]] bool HasWorld(
    const WorldScene& scene,
    const std::string& logicalName) {
    return std::any_of(
        scene.worlds.begin(),
        scene.worlds.end(),
        [&logicalName](const WorldRecord& world) {
            return world.logicalName == logicalName;
        });
}

[[nodiscard]] bool HasWorldNode(
    const WorldScene& scene,
    const std::string& logicalName) {
    return std::any_of(
        scene.worldNodes.begin(),
        scene.worldNodes.end(),
        [&logicalName](const WorldNodeRecord& node) {
            return node.logicalName == logicalName;
        });
}

} // namespace

bool WorldSceneExtractor::IsWorldResource(const std::string& logicalName) {
    return EndsWithIgnoreCase(logicalName, ".vwrld")
        || EndsWithIgnoreCase(logicalName, ".vwrld_c");
}

bool WorldSceneExtractor::IsWorldNodeResource(const std::string& logicalName) {
    return EndsWithIgnoreCase(logicalName, ".vwnod")
        || EndsWithIgnoreCase(logicalName, ".vwnod_c");
}

void WorldSceneExtractor::AddDocument(
    const std::string& logicalName,
    const serialization::kv3::Kv3Document& document,
    WorldScene& scene) const {
    scene.enabled = true;
    const auto& root = document.root;
    if (root.Type() != Kv3ValueType::Object) {
        scene.warnings.push_back(logicalName + ": KV3 root is not an object");
        return;
    }

    if (IsWorldResource(logicalName)) {
        if (HasWorld(scene, logicalName)) {
            return;
        }
        WorldRecord world;
        world.logicalName = logicalName;
        world.worldGroup = WorldGroupFromName(logicalName);
        world.entityLumps = ReadStringArray(
            root.Find("m_entityLumps"),
            world.warnings,
            "m_entityLumps");
        if (const auto* worldNodes = root.Find("m_worldNodes")) {
            if (worldNodes->Type() == Kv3ValueType::Array) {
                for (const auto& node : worldNodes->ArrayValues()) {
                    if (node.Type() != Kv3ValueType::Object) {
                        world.warnings.push_back("m_worldNodes contains a non-object value");
                        continue;
                    }
                    const auto prefix = StringProperty(node, "m_worldNodePrefix");
                    if (!prefix.empty()) {
                        world.worldNodePrefixes.push_back(prefix);
                    }
                }
            } else {
                world.warnings.push_back("m_worldNodes is not an array");
            }
        }
        world.hasWorldLightingInfo = root.Find("m_worldLightingInfo") != nullptr;
        scene.worlds.push_back(std::move(world));
