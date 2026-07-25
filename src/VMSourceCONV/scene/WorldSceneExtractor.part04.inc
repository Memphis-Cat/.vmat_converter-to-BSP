        return;
    }

    if (!IsWorldNodeResource(logicalName)) {
        return;
    }
    if (HasWorldNode(scene, logicalName)) {
        return;
    }

    WorldNodeRecord node;
    node.logicalName = logicalName;
    node.worldGroup = WorldGroupFromName(logicalName);
    node.layerNames = ReadStringArray(
        root.Find("m_layerNames"),
        node.warnings,
        "m_layerNames");
    const auto layerIndices = ReadIntegerArray(
        root.Find("m_sceneObjectLayerIndices"),
        node.warnings,
        "m_sceneObjectLayerIndices");

    if (const auto* sceneObjects = root.Find("m_sceneObjects")) {
        if (sceneObjects->Type() == Kv3ValueType::Array) {
            node.sceneObjects.reserve(sceneObjects->ArrayValues().size());
            for (std::size_t index = 0;
                 index < sceneObjects->ArrayValues().size();
                 ++index) {
                node.sceneObjects.push_back(ExtractSceneObject(
                    sceneObjects->ArrayValues()[index],
                    logicalName,
                    node.worldGroup,
                    index,
                    LayerIndexAt(layerIndices, index),
                    node.layerNames));
            }
        } else {
            node.warnings.push_back("m_sceneObjects is not an array");
        }
    } else {
        node.warnings.push_back("m_sceneObjects is missing");
    }

    if (const auto* aggregates = root.Find("m_aggregateSceneObjects")) {
        if (aggregates->Type() == Kv3ValueType::Array) {
            node.aggregates.reserve(aggregates->ArrayValues().size());
            for (std::size_t index = 0;
                 index < aggregates->ArrayValues().size();
                 ++index) {
                node.aggregates.push_back(ExtractAggregate(
                    aggregates->ArrayValues()[index],
                    logicalName,
                    node.worldGroup,
                    index,
                    node.layerNames));
            }
        } else {
            node.warnings.push_back("m_aggregateSceneObjects is not an array");
        }
    }

    if (const auto* clutter = root.Find("m_clutterSceneObjects")) {
        if (clutter->Type() == Kv3ValueType::Array) {
            node.clutterObjects.reserve(clutter->ArrayValues().size());
            for (std::size_t index = 0;
                 index < clutter->ArrayValues().size();
                 ++index) {
                node.clutterObjects.push_back(ExtractSceneObject(
                    clutter->ArrayValues()[index],
                    logicalName,
                    node.worldGroup,
                    index,
                    -1,
                    node.layerNames,
                    SceneObjectKind::Clutter));
            }
        } else {
            node.warnings.push_back("m_clutterSceneObjects is not an array");
        }
    }

    if (!layerIndices.empty()
        && layerIndices.size() != node.sceneObjects.size()) {
        node.warnings.push_back(
            "m_sceneObjectLayerIndices count does not match m_sceneObjects");
    }
    scene.worldNodes.push_back(std::move(node));
}

void WorldSceneExtractor::Finalize(WorldScene& scene) const {
    WorldSceneStatistics statistics;
    statistics.worldCount = scene.worlds.size();
    statistics.worldNodeCount = scene.worldNodes.size();
    std::unordered_set<std::string> renderables;

    for (const auto& node : scene.worldNodes) {
        statistics.layerCount += node.layerNames.size();
        statistics.sceneObjectCount += node.sceneObjects.size();
        statistics.aggregateCount += node.aggregates.size();
        statistics.clutterObjectCount += node.clutterObjects.size();

        for (const auto& object : node.sceneObjects) {
            if (!object.renderableModel.empty()) {
                ++statistics.modelInstanceCount;
                renderables.insert(object.renderableModel);
            }
            if (!object.renderableMesh.empty()) {
                ++statistics.meshInstanceCount;
                renderables.insert(object.renderableMesh);
            }
            if (object.transform.present) {
                ++statistics.transformedObjectCount;
            }
            if (object.bounds.present) {
                ++statistics.boundedObjectCount;
            }
        }
        for (const auto& object : node.clutterObjects) {
            if (!object.renderableModel.empty()) {
                renderables.insert(object.renderableModel);
            }
            if (!object.renderableMesh.empty()) {
                renderables.insert(object.renderableMesh);
            }
            if (object.transform.present) {
                ++statistics.transformedObjectCount;
            }
            if (object.bounds.present) {
                ++statistics.boundedObjectCount;
            }
        }
        for (const auto& aggregate : node.aggregates) {
            if (!aggregate.renderableModel.empty()) {
                renderables.insert(aggregate.renderableModel);
            }
            statistics.aggregateFragmentCount += aggregate.fragments.size();
            for (const auto& fragment : aggregate.fragments) {
                if (fragment.transform.present) {
                    ++statistics.transformedObjectCount;
                }
                if (fragment.bounds.present) {
                    ++statistics.boundedObjectCount;
                }
            }
        }
    }
    statistics.uniqueRenderableCount = renderables.size();
    scene.statistics = statistics;
}

} // namespace vmsourceconv::scene
