#include "serialization/kv3/Kv3Value.h"

#include <stdexcept>
#include <utility>

namespace vmsourceconv::serialization::kv3 {
namespace {

Kv3Value Make(const Kv3ValueType type) {
    Kv3Value value;
    switch (type) {
        case Kv3ValueType::Null: return Kv3Value::Null();
        case Kv3ValueType::Boolean: return Kv3Value::Boolean(false);
        case Kv3ValueType::SignedInteger: return Kv3Value::Signed(0);
        case Kv3ValueType::UnsignedInteger: return Kv3Value::Unsigned(0);
        case Kv3ValueType::FloatingPoint: return Kv3Value::Floating(0.0);
        case Kv3ValueType::String: return Kv3Value::String({});
        case Kv3ValueType::BinaryBlob: return Kv3Value::Binary({});
        case Kv3ValueType::Array: return Kv3Value::Array();
        case Kv3ValueType::Object: return Kv3Value::Object();
    }
    return value;
}

} // namespace

Kv3Value Kv3Value::Null() {
    return {};
}

Kv3Value Kv3Value::Boolean(const bool value) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::Boolean;
    result.booleanValue_ = value;
    return result;
}

Kv3Value Kv3Value::Signed(const std::int64_t value) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::SignedInteger;
    result.signedValue_ = value;
    return result;
}

Kv3Value Kv3Value::Unsigned(const std::uint64_t value) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::UnsignedInteger;
    result.unsignedValue_ = value;
    return result;
}

Kv3Value Kv3Value::Floating(const double value) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::FloatingPoint;
    result.floatingValue_ = value;
    return result;
}

Kv3Value Kv3Value::String(std::string value) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::String;
    result.stringValue_ = std::move(value);
    return result;
}

Kv3Value Kv3Value::Binary(std::vector<std::uint8_t> value) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::BinaryBlob;
    result.binaryValue_ = std::move(value);
    return result;
}

Kv3Value Kv3Value::Array(const std::size_t reserve) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::Array;
    result.children_.reserve(reserve);
    return result;
}

Kv3Value Kv3Value::Object(const std::size_t reserve) {
    auto result = Make(Kv3ValueType::Null);
    result.type_ = Kv3ValueType::Object;
    result.children_.reserve(reserve);
    result.objectKeys_.reserve(reserve);
    return result;
}

Kv3ValueType Kv3Value::Type() const noexcept { return type_; }
std::uint8_t Kv3Value::Flag() const noexcept { return flag_; }
void Kv3Value::SetFlag(const std::uint8_t flag) noexcept { flag_ = flag; }

bool Kv3Value::BooleanValue() const {
    RequireType(Kv3ValueType::Boolean);
    return booleanValue_;
}

std::int64_t Kv3Value::SignedValue() const {
    RequireType(Kv3ValueType::SignedInteger);
    return signedValue_;
}

std::uint64_t Kv3Value::UnsignedValue() const {
    RequireType(Kv3ValueType::UnsignedInteger);
    return unsignedValue_;
}

double Kv3Value::FloatingValue() const {
    RequireType(Kv3ValueType::FloatingPoint);
    return floatingValue_;
}

const std::string& Kv3Value::StringValue() const {
    RequireType(Kv3ValueType::String);
    return stringValue_;
}

const std::vector<std::uint8_t>& Kv3Value::BinaryValue() const {
    RequireType(Kv3ValueType::BinaryBlob);
    return binaryValue_;
}

const std::vector<Kv3Value>& Kv3Value::ArrayValues() const {
    RequireType(Kv3ValueType::Array);
    return children_;
}

std::vector<Kv3Value>& Kv3Value::ArrayValues() {
    RequireType(Kv3ValueType::Array);
    return children_;
}

const std::vector<std::string>& Kv3Value::ObjectKeys() const {
    RequireType(Kv3ValueType::Object);
    return objectKeys_;
}

const std::vector<Kv3Value>& Kv3Value::ObjectValues() const {
    RequireType(Kv3ValueType::Object);
    return children_;
}

std::vector<Kv3Value>& Kv3Value::ObjectValues() {
    RequireType(Kv3ValueType::Object);
    return children_;
}

void Kv3Value::AddArrayValue(Kv3Value value) {
    RequireType(Kv3ValueType::Array);
    children_.push_back(std::move(value));
}

void Kv3Value::AddProperty(std::string name, Kv3Value value) {
    RequireType(Kv3ValueType::Object);
    objectKeys_.push_back(std::move(name));
    children_.push_back(std::move(value));
}

const Kv3Value* Kv3Value::Find(const std::string_view name) const noexcept {
    if (type_ != Kv3ValueType::Object) {
        return nullptr;
    }
    for (std::size_t index = 0; index < objectKeys_.size(); ++index) {
        if (objectKeys_[index] == name) {
            return &children_[index];
        }
    }
    return nullptr;
}

Kv3Value* Kv3Value::Find(const std::string_view name) noexcept {
    if (type_ != Kv3ValueType::Object) {
        return nullptr;
    }
    for (std::size_t index = 0; index < objectKeys_.size(); ++index) {
        if (objectKeys_[index] == name) {
            return &children_[index];
        }
    }
    return nullptr;
}

std::size_t Kv3Value::ChildCount() const noexcept {
    return children_.size();
}

void Kv3Value::RequireType(const Kv3ValueType expected) const {
    if (type_ != expected) {
        throw std::logic_error("KV3 value has the wrong type");
    }
}

const char* ToString(const Kv3ValueType type) noexcept {
    switch (type) {
        case Kv3ValueType::Null: return "null";
        case Kv3ValueType::Boolean: return "boolean";
        case Kv3ValueType::SignedInteger: return "signed-integer";
        case Kv3ValueType::UnsignedInteger: return "unsigned-integer";
        case Kv3ValueType::FloatingPoint: return "floating-point";
        case Kv3ValueType::String: return "string";
        case Kv3ValueType::BinaryBlob: return "binary-blob";
        case Kv3ValueType::Array: return "array";
        case Kv3ValueType::Object: return "object";
    }
    return "unknown";
}

} // namespace vmsourceconv::serialization::kv3
