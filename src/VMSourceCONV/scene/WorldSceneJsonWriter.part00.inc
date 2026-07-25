#include "scene/WorldSceneJsonWriter.h"

#include "serialization/kv3/Kv3Document.h"
#include "serialization/kv3/Kv3JsonWriter.h"
#include "serialization/kv3/Kv3Value.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace vmsourceconv::scene {
namespace {

using serialization::kv3::Kv3Document;
using serialization::kv3::Kv3Value;

[[nodiscard]] Kv3Value MakeVector3(const Vector3& value) {
    auto result = Kv3Value::Array(3);
    result.AddArrayValue(Kv3Value::Floating(value.x));
    result.AddArrayValue(Kv3Value::Floating(value.y));
    result.AddArrayValue(Kv3Value::Floating(value.z));
    return result;
}

[[nodiscard]] Kv3Value MakeVector4(const Vector4& value) {
    auto result = Kv3Value::Array(4);
    result.AddArrayValue(Kv3Value::Floating(value.x));
    result.AddArrayValue(Kv3Value::Floating(value.y));
    result.AddArrayValue(Kv3Value::Floating(value.z));
    result.AddArrayValue(Kv3Value::Floating(value.w));
    return result;
}

[[nodiscard]] Kv3Value MakeQuaternion(const Quaternion& value) {
    auto result = Kv3Value::Array(4);
    result.AddArrayValue(Kv3Value::Floating(value.x));
    result.AddArrayValue(Kv3Value::Floating(value.y));
    result.AddArrayValue(Kv3Value::Floating(value.z));
    result.AddArrayValue(Kv3Value::Floating(value.w));
    return result;
}

[[nodiscard]] Kv3Value MakeStrings(const std::vector<std::string>& values) {
    auto result = Kv3Value::Array(values.size());
    for (const auto& value : values) {
        result.AddArrayValue(Kv3Value::String(value));
    }
    return result;
}

[[nodiscard]] Kv3Value MakeTransform(const Transform& transform) {
    auto result = Kv3Value::Object();
    result.AddProperty("present", Kv3Value::Boolean(transform.present));
    result.AddProperty(
        "decompositionSucceeded",
        Kv3Value::Boolean(transform.decompositionSucceeded));

    auto matrix = Kv3Value::Array(transform.matrix.size());
    for (const auto number : transform.matrix) {
        matrix.AddArrayValue(Kv3Value::Floating(number));
    }
    result.AddProperty("matrixRowMajor", std::move(matrix));
    result.AddProperty("origin", MakeVector3(transform.origin));
    result.AddProperty("scale", MakeVector3(transform.scale));
    result.AddProperty("rotationQuaternion", MakeQuaternion(transform.rotation));
    return result;
}

[[nodiscard]] Kv3Value MakeBounds(const Bounds& bounds) {
    auto result = Kv3Value::Object();
    result.AddProperty("present", Kv3Value::Boolean(bounds.present));
    if (bounds.present) {
        result.AddProperty("minimum", MakeVector3(bounds.minimum));
        result.AddProperty("maximum", MakeVector3(bounds.maximum));
    }
    return result;
}

void AddFlags(
    Kv3Value& output,
    const bool hasNumeric,
    const std::uint64_t numeric,
    const std::string& text) {
    if (hasNumeric) {
        output.AddProperty("objectFlags", Kv3Value::Unsigned(numeric));
    } else if (!text.empty()) {
        output.AddProperty("objectFlagsText", Kv3Value::String(text));
    }
}

[[nodiscard]] Kv3Value MakeSceneObject(const SceneObjectRecord& object) {
    auto result = Kv3Value::Object();
    result.AddProperty("kind", Kv3Value::String(ToString(object.kind)));
    result.AddProperty("sourceIndex", Kv3Value::Unsigned(object.sourceIndex));
    result.AddProperty("sourceResource", Kv3Value::String(object.sourceResource));
    result.AddProperty("worldGroup", Kv3Value::String(object.worldGroup));
    result.AddProperty("renderableModel", Kv3Value::String(object.renderableModel));
    result.AddProperty("renderableMesh", Kv3Value::String(object.renderableMesh));
    result.AddProperty("skin", Kv3Value::String(object.skin));
    result.AddProperty("layerIndex", Kv3Value::Signed(object.layerIndex));
    result.AddProperty("layerName", Kv3Value::String(object.layerName));
    result.AddProperty("transform", MakeTransform(object.transform));
    result.AddProperty("bounds", MakeBounds(object.bounds));
    if (object.hasTint) {
        result.AddProperty("tint", MakeVector4(object.tint));
    }
    if (object.hasLightingOrigin) {
        result.AddProperty("lightingOrigin", MakeVector3(object.lightingOrigin));
    }
    AddFlags(
        result,
        object.hasNumericFlags,
        object.numericFlags,
        object.flagsText);
    result.AddProperty(
        "fadeStartDistance",
        Kv3Value::Floating(object.fadeStartDistance));
    result.AddProperty(
        "fadeEndDistance",
        Kv3Value::Floating(object.fadeEndDistance));
    result.AddProperty("warnings", MakeStrings(object.warnings));
    return result;
}

[[nodiscard]] Kv3Value MakeFragment(const AggregateFragmentRecord& fragment) {
    auto result = Kv3Value::Object();
    result.AddProperty("sourceIndex", Kv3Value::Unsigned(fragment.sourceIndex));
    result.AddProperty("drawCallIndex", Kv3Value::Signed(fragment.drawCallIndex));
    result.AddProperty("lodGroupMask", Kv3Value::Unsigned(fragment.lodGroupMask));
    result.AddProperty("hasTransform", Kv3Value::Boolean(fragment.hasTransform));
    result.AddProperty("transform", MakeTransform(fragment.transform));
    result.AddProperty("bounds", MakeBounds(fragment.bounds));
    if (fragment.hasTint) {
        result.AddProperty("tint", MakeVector4(fragment.tint));
    }
    AddFlags(
        result,
        fragment.hasNumericFlags,
        fragment.numericFlags,
        fragment.flagsText);
    result.AddProperty("warnings", MakeStrings(fragment.warnings));
    return result;
}

