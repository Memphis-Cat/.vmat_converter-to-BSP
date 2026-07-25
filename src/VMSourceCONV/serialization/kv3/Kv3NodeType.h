#pragma once

#include <cstdint>

namespace vmsourceconv::serialization::kv3 {

enum class Kv3NodeType : std::uint8_t {
    Null = 1,
    Boolean = 2,
    Int64 = 3,
    UInt64 = 4,
    Double = 5,
    String = 6,
    BinaryBlob = 7,
    Array = 8,
    Object = 9,
    TypedArray = 10,
    Int32 = 11,
    UInt32 = 12,
    BooleanTrue = 13,
    BooleanFalse = 14,
    Int64Zero = 15,
    Int64One = 16,
    DoubleZero = 17,
    DoubleOne = 18,
    Float = 19,
    Int16 = 20,
    UInt16 = 21,
    Unknown22 = 22,
    Int32AsByte = 23,
    TypedArrayByteLength = 24,
    TypedArrayAuxiliaryBuffer = 25,
};

[[nodiscard]] const char* ToString(Kv3NodeType type) noexcept;

} // namespace vmsourceconv::serialization::kv3
