#include "entities/EntityLumpJsonWriter.h"

#include "serialization/kv3/Kv3JsonWriter.h"

#include <fstream>
#include <stdexcept>
#include <utility>

namespace vmsourceconv::entities {
namespace {

using serialization::kv3::Kv3Value;

Kv3Value MakeLumpValue(const EntityLumpExport& lump) {
    auto value = Kv3Value::Object();
    value.AddProperty("resource", Kv3Value::String(lump.logicalName));
    value.AddProperty("name", Kv3Value::String(lump.lumpName));

    auto childLumps = Kv3Value::Array(lump.childLumps.size());
    for (const auto& child : lump.childLumps) {
        childLumps.AddArrayValue(Kv3Value::String(child));
    }
    value.AddProperty("childLumps", std::move(childLumps));

    auto entities = Kv3Value::Array(lump.entities.size());
    for (const auto& entity : lump.entities) {
        entities.AddArrayValue(entity);
    }
    value.AddProperty("entities", std::move(entities));

    auto summary = Kv3Value::Object();
    summary.AddProperty("entityCount", Kv3Value::Unsigned(lump.entities.size()));
    summary.AddProperty(
        "skippedWithoutClassname",
        Kv3Value::Unsigned(lump.skippedWithoutClassname));
    summary.AddProperty(
        "preservedBinaryEntities",
        Kv3Value::Unsigned(lump.preservedBinaryEntities));
    value.AddProperty("summary", std::move(summary));

    auto warnings = Kv3Value::Array(lump.warnings.size());
    for (const auto& warning : lump.warnings) {
        warnings.AddArrayValue(Kv3Value::String(warning));
    }
    value.AddProperty("warnings", std::move(warnings));
    return value;
}

} // namespace

void EntityLumpJsonWriter::Write(
    const std::vector<EntityLumpExport>& lumps,
    std::ostream& output) const {
    auto root = Kv3Value::Object();
    auto values = Kv3Value::Array(lumps.size());
    for (const auto& lump : lumps) {
        values.AddArrayValue(MakeLumpValue(lump));
    }
    root.AddProperty("lumps", std::move(values));
    serialization::kv3::Kv3JsonWriter{}.WriteValue(root, output);
}

void EntityLumpJsonWriter::WriteFile(
    const std::vector<EntityLumpExport>& lumps,
    const std::filesystem::path& path) const {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error(
            "unable to open entity JSON file: " + path.string());
    }
    Write(lumps, stream);
    if (!stream) {
        throw std::runtime_error(
            "unable to write entity JSON file: " + path.string());
    }
}

} // namespace vmsourceconv::entities
