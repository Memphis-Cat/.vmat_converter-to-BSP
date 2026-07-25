#include "entities/EntityLumpExport.h"

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace vmsourceconv::entities {
namespace {

using serialization::kv3::Kv3Value;
using serialization::kv3::Kv3ValueType;

[[nodiscard]] std::int64_t IntegerValue(const Kv3Value& value) {
    if (value.Type() == Kv3ValueType::SignedInteger) {
        return value.SignedValue();
    }
    if (value.Type() == Kv3ValueType::UnsignedInteger) {
        const auto unsignedValue = value.UnsignedValue();
        if (unsignedValue > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max())) {
            throw std::runtime_error("entity version exceeds int64");
        }
        return static_cast<std::int64_t>(unsignedValue);
    }
    throw std::runtime_error("entity version is not an integer");
}

void CopyObjectProperties(const Kv3Value* source, Kv3Value& destination) {
    if (source == nullptr || source->Type() == Kv3ValueType::Null) {
        return;
    }
    if (source->Type() != Kv3ValueType::Object) {
        throw std::runtime_error("entity values or attributes are not an object");
    }

    const auto& keys = source->ObjectKeys();
    const auto& values = source->ObjectValues();
    for (std::size_t index = 0; index < values.size(); ++index) {
        destination.AddProperty(keys[index], values[index]);
    }
}

[[nodiscard]] Kv3Value ExtractEntityKv3(const Kv3Value& source) {
    const auto* version = source.Find("version");
    if (version == nullptr || IntegerValue(*version) != 1) {
        throw std::runtime_error("unsupported KV3 entity payload version");
    }

    auto entity = Kv3Value::Object();
    CopyObjectProperties(source.Find("values"), entity);
    CopyObjectProperties(source.Find("attributes"), entity);
    return entity;
}

} // namespace

EntityLumpExport EntityLumpExtractor::Extract(
    std::string logicalName,
    const serialization::kv3::Kv3Document& document) const {
    EntityLumpExport result;
    result.logicalName = std::move(logicalName);

    const auto& root = document.root;
    if (root.Type() != Kv3ValueType::Object) {
        throw std::runtime_error("entity lump root is not an object");
    }

    if (const auto* name = root.Find("m_name");
        name != nullptr && name->Type() == Kv3ValueType::String) {
        result.lumpName = name->StringValue();
    }

    if (const auto* children = root.Find("m_childLumps");
        children != nullptr && children->Type() == Kv3ValueType::Array) {
        for (const auto& child : children->ArrayValues()) {
            if (child.Type() == Kv3ValueType::String) {
                result.childLumps.push_back(child.StringValue());
            }
        }
    }

    const auto* entityValues = root.Find("m_entityKeyValues");
    if (entityValues == nullptr || entityValues->Type() != Kv3ValueType::Array) {
        throw std::runtime_error("entity lump does not contain m_entityKeyValues array");
    }

    result.entities.reserve(entityValues->ArrayValues().size());
    for (std::size_t index = 0; index < entityValues->ArrayValues().size(); ++index) {
        const auto& wrapper = entityValues->ArrayValues()[index];
        if (wrapper.Type() != Kv3ValueType::Object) {
            result.warnings.push_back(
                "entity wrapper " + std::to_string(index) + " is not an object");
            continue;
        }

        Kv3Value entity;
        if (const auto* kv3Data = wrapper.Find("keyValues3Data");
            kv3Data != nullptr && kv3Data->Type() == Kv3ValueType::Object) {
            entity = ExtractEntityKv3(*kv3Data);
        } else if (const auto* binaryData = wrapper.Find("m_keyValuesData");
                   binaryData != nullptr
                   && binaryData->Type() == Kv3ValueType::BinaryBlob) {
            entity = Kv3Value::Object();
            entity.AddProperty("_rawKeyValuesData", *binaryData);
            ++result.preservedBinaryEntities;
        } else {
            result.warnings.push_back(
                "entity wrapper " + std::to_string(index)
                + " has no supported property payload");
            continue;
        }

        if (const auto* connections = wrapper.Find("m_connections");
            connections != nullptr
            && connections->Type() == Kv3ValueType::Array
            && !connections->ArrayValues().empty()) {
            entity.AddProperty("_connections", *connections);
        }
        entity.AddProperty("_sourceIndex", Kv3Value::Unsigned(index));

        if (entity.Find("classname") == nullptr
            && entity.Find("_rawKeyValuesData") == nullptr) {
            ++result.skippedWithoutClassname;
            continue;
        }
        result.entities.push_back(std::move(entity));
    }

    return result;
}

} // namespace vmsourceconv::entities
