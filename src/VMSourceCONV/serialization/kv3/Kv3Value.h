#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

enum class Kv3ValueType {
    Null,
    Boolean,
    SignedInteger,
    UnsignedInteger,
    FloatingPoint,
    String,
    BinaryBlob,
    Array,
    Object,
};

class Kv3Value final {
public:
    Kv3Value() = default;

    [[nodiscard]] static Kv3Value Null();
    [[nodiscard]] static Kv3Value Boolean(bool value);
    [[nodiscard]] static Kv3Value Signed(std::int64_t value);
    [[nodiscard]] static Kv3Value Unsigned(std::uint64_t value);
    [[nodiscard]] static Kv3Value Floating(double value);
    [[nodiscard]] static Kv3Value String(std::string value);
    [[nodiscard]] static Kv3Value Binary(std::vector<std::uint8_t> value);
    [[nodiscard]] static Kv3Value Array(std::size_t reserve = 0);
    [[nodiscard]] static Kv3Value Object(std::size_t reserve = 0);

    [[nodiscard]] Kv3ValueType Type() const noexcept;
    [[nodiscard]] std::uint8_t Flag() const noexcept;
    void SetFlag(std::uint8_t flag) noexcept;

    [[nodiscard]] bool BooleanValue() const;
    [[nodiscard]] std::int64_t SignedValue() const;
    [[nodiscard]] std::uint64_t UnsignedValue() const;
    [[nodiscard]] double FloatingValue() const;
    [[nodiscard]] const std::string& StringValue() const;
    [[nodiscard]] const std::vector<std::uint8_t>& BinaryValue() const;
    [[nodiscard]] const std::vector<Kv3Value>& ArrayValues() const;
    [[nodiscard]] std::vector<Kv3Value>& ArrayValues();
    [[nodiscard]] const std::vector<std::string>& ObjectKeys() const;
    [[nodiscard]] const std::vector<Kv3Value>& ObjectValues() const;
    [[nodiscard]] std::vector<Kv3Value>& ObjectValues();

    void AddArrayValue(Kv3Value value);
    void AddProperty(std::string name, Kv3Value value);

    [[nodiscard]] const Kv3Value* Find(std::string_view name) const noexcept;
    [[nodiscard]] Kv3Value* Find(std::string_view name) noexcept;
    [[nodiscard]] std::size_t ChildCount() const noexcept;

private:
    void RequireType(Kv3ValueType expected) const;

    Kv3ValueType type_ = Kv3ValueType::Null;
    std::uint8_t flag_ = 0;
    bool booleanValue_ = false;
    std::int64_t signedValue_ = 0;
    std::uint64_t unsignedValue_ = 0;
    double floatingValue_ = 0.0;
    std::string stringValue_;
    std::vector<std::uint8_t> binaryValue_;
    std::vector<Kv3Value> children_;
    std::vector<std::string> objectKeys_;
};

[[nodiscard]] const char* ToString(Kv3ValueType type) noexcept;

} // namespace vmsourceconv::serialization::kv3
