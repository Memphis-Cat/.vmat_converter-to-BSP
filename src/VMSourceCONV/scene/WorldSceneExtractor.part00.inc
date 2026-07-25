#include "scene/WorldSceneExtractor.h"

#include "serialization/kv3/Kv3Value.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vmsourceconv::scene {
namespace {

using serialization::kv3::Kv3Value;
using serialization::kv3::Kv3ValueType;

[[nodiscard]] std::string LowerCopy(const std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        result.push_back(static_cast<char>(std::tolower(character)));
    }
    return result;
}

[[nodiscard]] bool EndsWithIgnoreCase(
    const std::string_view value,
    const std::string_view suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(
        suffix.rbegin(),
        suffix.rend(),
        value.rbegin(),
        [](const unsigned char left, const unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        });
}

[[nodiscard]] std::string NormalizePath(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    while (!value.empty() && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

[[nodiscard]] std::string WorldGroupFromName(const std::string& logicalName) {
    const auto normalized = NormalizePath(logicalName);
    const auto lower = LowerCopy(normalized);
    const auto nodeMarker = lower.find("/worldnodes/");
    if (nodeMarker != std::string::npos) {
        return normalized.substr(0, nodeMarker);
    }
    const auto worldMarker = lower.rfind("/world.vwrld");
    if (worldMarker != std::string::npos) {
        return normalized.substr(0, worldMarker);
    }
    const auto slash = normalized.rfind('/');
    return slash == std::string::npos ? std::string{} : normalized.substr(0, slash);
}

[[nodiscard]] bool TryDouble(const Kv3Value& value, double& output) {
    switch (value.Type()) {
        case Kv3ValueType::SignedInteger:
            output = static_cast<double>(value.SignedValue());
            return true;
        case Kv3ValueType::UnsignedInteger:
            output = static_cast<double>(value.UnsignedValue());
            return true;
        case Kv3ValueType::FloatingPoint:
            output = value.FloatingValue();
            return true;
        default:
            return false;
    }
}

[[nodiscard]] bool TrySigned(const Kv3Value& value, std::int64_t& output) {
    if (value.Type() == Kv3ValueType::SignedInteger) {
        output = value.SignedValue();
        return true;
    }
    if (value.Type() == Kv3ValueType::UnsignedInteger
        && value.UnsignedValue()
            <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        output = static_cast<std::int64_t>(value.UnsignedValue());
        return true;
    }
    return false;
}

[[nodiscard]] bool TryUnsigned(const Kv3Value& value, std::uint64_t& output) {
    if (value.Type() == Kv3ValueType::UnsignedInteger) {
        output = value.UnsignedValue();
        return true;
    }
    if (value.Type() == Kv3ValueType::SignedInteger
        && value.SignedValue() >= 0) {
        output = static_cast<std::uint64_t>(value.SignedValue());
        return true;
    }
    return false;
}

[[nodiscard]] bool TryBoolean(const Kv3Value& value, bool& output) {
    if (value.Type() != Kv3ValueType::Boolean) {
        return false;
    }
    output = value.BooleanValue();
    return true;
}

[[nodiscard]] std::string StringProperty(
    const Kv3Value& object,
    const std::string_view name) {
    const auto* value = object.Find(name);
    if (value == nullptr || value->Type() != Kv3ValueType::String) {
        return {};
    }
    return value->StringValue();
}

[[nodiscard]] bool TryVector3(const Kv3Value& value, Vector3& output) {
    if (value.Type() != Kv3ValueType::Array
        || value.ArrayValues().size() < 3U) {
        return false;
    }
    return TryDouble(value.ArrayValues()[0], output.x)
        && TryDouble(value.ArrayValues()[1], output.y)
        && TryDouble(value.ArrayValues()[2], output.z);
}

[[nodiscard]] bool TryVector4(const Kv3Value& value, Vector4& output) {
    if (value.Type() != Kv3ValueType::Array
        || value.ArrayValues().size() < 3U) {
        return false;
    }
    if (!TryDouble(value.ArrayValues()[0], output.x)
        || !TryDouble(value.ArrayValues()[1], output.y)
        || !TryDouble(value.ArrayValues()[2], output.z)) {
        return false;
    }
    output.w = 1.0;
    if (value.ArrayValues().size() >= 4U) {
        return TryDouble(value.ArrayValues()[3], output.w);
    }
    return true;
}

[[nodiscard]] bool TryNumericArray(
    const Kv3Value& value,
    std::vector<double>& output) {
    if (value.Type() != Kv3ValueType::Array) {
        return false;
    }
    output.clear();
    output.reserve(value.ArrayValues().size());
    for (const auto& item : value.ArrayValues()) {
        double number = 0.0;
        if (!TryDouble(item, number)) {
            output.clear();
