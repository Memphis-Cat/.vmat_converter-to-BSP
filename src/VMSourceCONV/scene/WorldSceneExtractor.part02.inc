    }
    return false;
}

void ReadFlags(
    const Kv3Value* value,
    bool& hasNumericFlags,
    std::uint64_t& numericFlags,
    std::string& flagsText) {
    if (value == nullptr) {
        return;
    }
    if (TryUnsigned(*value, numericFlags)) {
        hasNumericFlags = true;
        return;
    }
    if (value->Type() == Kv3ValueType::String) {
        flagsText = value->StringValue();
    }
}

[[nodiscard]] std::int64_t LayerIndexAt(
    const std::vector<std::int64_t>& indices,
    const std::size_t index) {
    return index < indices.size() ? indices[index] : -1;
}

[[nodiscard]] std::string LayerNameAt(
    const std::vector<std::string>& layers,
    const std::int64_t index) {
    if (index < 0 || static_cast<std::size_t>(index) >= layers.size()) {
        return index < 0 ? "No layer" : std::string{};
    }
    return layers[static_cast<std::size_t>(index)];
}

[[nodiscard]] std::vector<std::string> ReadStringArray(
    const Kv3Value* value,
    std::vector<std::string>& warnings,
    const std::string_view fieldName) {
    std::vector<std::string> result;
    if (value == nullptr) {
        return result;
    }
    if (value->Type() != Kv3ValueType::Array) {
        warnings.push_back(std::string(fieldName) + " is not an array");
        return result;
    }
    result.reserve(value->ArrayValues().size());
    for (const auto& item : value->ArrayValues()) {
        if (item.Type() == Kv3ValueType::String) {
            result.push_back(item.StringValue());
        } else {
            warnings.push_back(std::string(fieldName) + " contains a non-string value");
        }
    }
    return result;
}

[[nodiscard]] std::vector<std::int64_t> ReadIntegerArray(
    const Kv3Value* value,
    std::vector<std::string>& warnings,
    const std::string_view fieldName) {
    std::vector<std::int64_t> result;
    if (value == nullptr) {
        return result;
    }
    if (value->Type() != Kv3ValueType::Array) {
        warnings.push_back(std::string(fieldName) + " is not an array");
        return result;
    }
    result.reserve(value->ArrayValues().size());
    for (const auto& item : value->ArrayValues()) {
        std::int64_t integer = 0;
        if (TrySigned(item, integer)) {
            result.push_back(integer);
        } else {
            warnings.push_back(std::string(fieldName) + " contains a non-integer value");
            result.push_back(-1);
        }
    }
    return result;
}

[[nodiscard]] SceneObjectRecord ExtractSceneObject(
    const Kv3Value& source,
    const std::string& logicalName,
    const std::string& worldGroup,
    const std::size_t sourceIndex,
    const std::int64_t layerIndex,
    const std::vector<std::string>& layers,
    const SceneObjectKind forcedKind = SceneObjectKind::Model) {
    SceneObjectRecord result;
    result.sourceResource = logicalName;
    result.worldGroup = worldGroup;
    result.sourceIndex = sourceIndex;
    result.layerIndex = layerIndex;
    result.layerName = LayerNameAt(layers, layerIndex);

    if (source.Type() != Kv3ValueType::Object) {
        result.kind = forcedKind;
        result.warnings.push_back("scene object is not an object");
        return result;
    }

    result.renderableModel = StringProperty(source, "m_renderableModel");
    result.renderableMesh = StringProperty(source, "m_renderable");
    result.skin = StringProperty(source, "m_skin");
    if (forcedKind == SceneObjectKind::Clutter) {
        result.kind = SceneObjectKind::Clutter;
    } else {
        result.kind = result.renderableModel.empty()
            ? SceneObjectKind::Mesh
            : SceneObjectKind::Model;
    }

    if (const auto* transform = source.Find("m_vTransform");
        transform != nullptr && !TryTransform(*transform, result.transform)) {
        result.warnings.push_back("m_vTransform has an unsupported layout");
    }
    if (const auto* tint = source.Find("m_vTintColor");
        tint != nullptr) {
        result.hasTint = TryVector4(*tint, result.tint);
        if (!result.hasTint) {
            result.warnings.push_back("m_vTintColor is not a numeric vector");
        }
    }
    if (const auto* lighting = source.Find("m_vLightingOrigin");
        lighting != nullptr) {
        result.hasLightingOrigin = TryVector3(*lighting, result.lightingOrigin);
        if (!result.hasLightingOrigin) {
            result.warnings.push_back("m_vLightingOrigin is not a numeric vector");
        }
    }

    ReadFlags(
        source.Find("m_nObjectTypeFlags"),
        result.hasNumericFlags,
        result.numericFlags,
        result.flagsText);

    if (const auto* fadeStart = source.Find("m_flFadeStartDistance")) {
        (void)TryDouble(*fadeStart, result.fadeStartDistance);
    }
    if (const auto* fadeEnd = source.Find("m_flFadeEndDistance")) {
        (void)TryDouble(*fadeEnd, result.fadeEndDistance);
    }

    const std::array<std::string_view, 3> boundsFields{
        "m_vWorldBounds", "m_vBounds", "m_bounds",
    };
    for (const auto field : boundsFields) {
        const auto* bounds = source.Find(field);
        if (bounds != nullptr && TryBounds(*bounds, result.bounds)) {
            break;
        }
    }

    if (result.renderableModel.empty() && result.renderableMesh.empty()) {
        result.warnings.push_back("scene object has no renderable model or mesh");
    }
    return result;
}

[[nodiscard]] AggregateRecord ExtractAggregate(
    const Kv3Value& source,
    const std::string& logicalName,
    const std::string& worldGroup,
    const std::size_t sourceIndex,
    const std::vector<std::string>& layers) {
