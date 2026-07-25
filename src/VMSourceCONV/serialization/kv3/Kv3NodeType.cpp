#include "serialization/kv3/Kv3NodeType.h"

namespace vmsourceconv::serialization::kv3 {

const char* ToString(const Kv3NodeType type) noexcept {
    switch (type) {
        case Kv3NodeType::Null: return "null";
        case Kv3NodeType::Boolean: return "boolean";
        case Kv3NodeType::Int64: return "int64";
        case Kv3NodeType::UInt64: return "uint64";
        case Kv3NodeType::Double: return "double";
        case Kv3NodeType::String: return "string";
        case Kv3NodeType::BinaryBlob: return "binary-blob";
        case Kv3NodeType::Array: return "array";
        case Kv3NodeType::Object: return "object";
        case Kv3NodeType::TypedArray: return "typed-array";
        case Kv3NodeType::Int32: return "int32";
        case Kv3NodeType::UInt32: return "uint32";
        case Kv3NodeType::BooleanTrue: return "boolean-true";
        case Kv3NodeType::BooleanFalse: return "boolean-false";
        case Kv3NodeType::Int64Zero: return "int64-zero";
        case Kv3NodeType::Int64One: return "int64-one";
        case Kv3NodeType::DoubleZero: return "double-zero";
        case Kv3NodeType::DoubleOne: return "double-one";
        case Kv3NodeType::Float: return "float";
        case Kv3NodeType::Int16: return "int16";
        case Kv3NodeType::UInt16: return "uint16";
        case Kv3NodeType::Unknown22: return "unknown-22";
        case Kv3NodeType::Int32AsByte: return "int32-as-byte";
        case Kv3NodeType::TypedArrayByteLength: return "typed-array-byte-length";
        case Kv3NodeType::TypedArrayAuxiliaryBuffer: return "typed-array-auxiliary-buffer";
    }
    return "unknown";
}

} // namespace vmsourceconv::serialization::kv3
