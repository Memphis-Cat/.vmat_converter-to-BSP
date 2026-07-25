#include "serialization/kv3/BinaryKv3Decoder.h"

#include "serialization/kv3/BinaryKv3HeaderReader.h"
#include "serialization/kv3/Kv3NodeType.h"
#include "serialization/kv3/Kv3PayloadDecoder.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vmsourceconv::serialization::kv3 {
namespace {

struct TypeAndFlag {
    Kv3NodeType type = Kv3NodeType::Null;
    std::uint8_t flag = 0;
};

class ValueDecoder final {
public:
    ValueDecoder(Kv3Payload payload, const Kv3DecodeLimits& limits)
        : payload_(std::move(payload)), limits_(limits) {
        statistics_.stringCount = payload_.strings.size();
    }

    [[nodiscard]] Kv3Document Decode() {
        const auto rootType = ReadType();
        auto root = ReadValue(rootType, 0);

        if (typePosition_ != payload_.typeStream.size()) {
            throw std::runtime_error("KV3 type stream was not fully consumed");
        }
        if (objectLengthPosition_ != payload_.objectLengths.size()) {
            throw std::runtime_error("KV3 object length stream was not fully consumed");
        }
        if (binaryBlobLengthPosition_ != payload_.binaryBlobLengths.size()) {
            throw std::runtime_error("KV3 binary blob length stream was not fully consumed");
        }
        if (binaryBlobPosition_ != payload_.binaryBlobs.size()) {
            throw std::runtime_error("KV3 binary blob data was not fully consumed");
        }
        if (payload_.mainBuffer.Remaining1() != 0
            || payload_.mainBuffer.Remaining2() != 0
            || payload_.mainBuffer.Remaining4() != 0
            || payload_.mainBuffer.Remaining8() != 0) {
            throw std::runtime_error("KV3 main scalar buffers were not fully consumed");
        }
        if (payload_.auxiliaryBuffer.Remaining1() != 0
            || payload_.auxiliaryBuffer.Remaining2() != 0
            || payload_.auxiliaryBuffer.Remaining4() != 0
            || payload_.auxiliaryBuffer.Remaining8() != 0) {
            throw std::runtime_error("KV3 auxiliary scalar buffers were not fully consumed");
        }

        Kv3Document document;
        document.formatId = payload_.header.formatId;
        document.root = std::move(root);
        document.statistics = statistics_;
        return document;
    }

private:
    [[nodiscard]] TypeAndFlag ReadType() {
        if (typePosition_ >= payload_.typeStream.size()) {
            throw std::runtime_error("KV3 type stream is truncated");
        }

        auto typeByte = payload_.typeStream[typePosition_++];
        std::uint8_t flag = 0;
        if ((typeByte & 0x80U) != 0U) {
            typeByte &= 0x3FU;
            if (typePosition_ >= payload_.typeStream.size()) {
                throw std::runtime_error("KV3 flagged type is missing its flag byte");
            }
            flag = payload_.typeStream[typePosition_++];
        }

        if (typeByte < static_cast<std::uint8_t>(Kv3NodeType::Null)
            || typeByte > static_cast<std::uint8_t>(Kv3NodeType::TypedArrayAuxiliaryBuffer)) {
            throw std::runtime_error(
                "unknown KV3 node type " + std::to_string(typeByte));
        }
        return {static_cast<Kv3NodeType>(typeByte), flag};
    }

    [[nodiscard]] Kv3Value ReadValue(
        const TypeAndFlag typeAndFlag,
        const std::size_t depth) {
        if (depth > limits_.maximumDepth) {
            throw std::runtime_error("KV3 nesting exceeds the configured depth limit");
        }
        if (statistics_.nodeCount >= limits_.maximumNodes) {
            throw std::runtime_error("KV3 node count exceeds the configured limit");
        }
        ++statistics_.nodeCount;
        statistics_.maximumDepth = std::max(statistics_.maximumDepth, depth);

        auto value = ReadUnflaggedValue(typeAndFlag.type, depth);
        value.SetFlag(typeAndFlag.flag);
        return value;
    }

    [[nodiscard]] Kv3Value ReadUnflaggedValue(
        const Kv3NodeType type,
        const std::size_t depth) {
        switch (type) {
            case Kv3NodeType::Null:
                return Kv3Value::Null();
            case Kv3NodeType::BooleanTrue:
                return Kv3Value::Boolean(true);
            case Kv3NodeType::BooleanFalse:
                return Kv3Value::Boolean(false);
            case Kv3NodeType::Int64Zero:
                return Kv3Value::Signed(0);
            case Kv3NodeType::Int64One:
                return Kv3Value::Signed(1);
            case Kv3NodeType::DoubleZero:
                return Kv3Value::Floating(0.0);
            case Kv3NodeType::DoubleOne:
                return Kv3Value::Floating(1.0);
            case Kv3NodeType::Boolean:
                return Kv3Value::Boolean(CurrentBuffer().ReadU8() == 1U);
            case Kv3NodeType::Int32AsByte:
                return Kv3Value::Signed(CurrentBuffer().ReadU8());
            case Kv3NodeType::Unknown22:
                throw std::runtime_error("KV3 node type 22 is unsupported");
            case Kv3NodeType::Int16:
                return Kv3Value::Signed(CurrentBuffer().ReadI16());
            case Kv3NodeType::UInt16:
                return Kv3Value::Unsigned(CurrentBuffer().ReadU16());
            case Kv3NodeType::Int32:
                return Kv3Value::Signed(CurrentBuffer().ReadI32());
            case Kv3NodeType::UInt32:
                return Kv3Value::Unsigned(CurrentBuffer().ReadU32());
            case Kv3NodeType::Float:
                return Kv3Value::Floating(CurrentBuffer().ReadFloat());
            case Kv3NodeType::Int64:
                return Kv3Value::Signed(CurrentBuffer().ReadI64());
            case Kv3NodeType::UInt64:
                return Kv3Value::Unsigned(CurrentBuffer().ReadU64());
            case Kv3NodeType::Double:
                return Kv3Value::Floating(CurrentBuffer().ReadDouble());
            case Kv3NodeType::String:
                return Kv3Value::String(ReadStringById(CurrentBuffer().ReadI32()));
            case Kv3NodeType::BinaryBlob:
                return ReadBinaryBlob();
            case Kv3NodeType::Array:
                return ReadArray(depth);
            case Kv3NodeType::TypedArray:
                return ReadTypedArray(CurrentBuffer().ReadI32(), false, depth);
            case Kv3NodeType::TypedArrayByteLength:
                return ReadTypedArray(CurrentBuffer().ReadU8(), false, depth);
            case Kv3NodeType::TypedArrayAuxiliaryBuffer:
                return ReadTypedArray(CurrentBuffer().ReadU8(), true, depth);
            case Kv3NodeType::Object:
                return ReadObject(depth);
        }

        throw std::runtime_error(
            std::string("unsupported KV3 node type ") + ToString(type));
    }

    [[nodiscard]] Kv3Value ReadBinaryBlob() {
        if (binaryBlobLengthPosition_ >= payload_.binaryBlobLengths.size()) {
            throw std::runtime_error("KV3 binary blob length stream is truncated");
        }
        const auto length = payload_.binaryBlobLengths[binaryBlobLengthPosition_++];
        if (length < 0) {
            throw std::runtime_error("KV3 binary blob length is negative");
        }
        const auto size = static_cast<std::size_t>(length);
        if (binaryBlobPosition_ > payload_.binaryBlobs.size()
            || size > payload_.binaryBlobs.size() - binaryBlobPosition_) {
            throw std::runtime_error("KV3 binary blob exceeds its data stream");
        }

        std::vector<std::uint8_t> bytes(
            payload_.binaryBlobs.begin() + static_cast<std::ptrdiff_t>(binaryBlobPosition_),
            payload_.binaryBlobs.begin() + static_cast<std::ptrdiff_t>(binaryBlobPosition_ + size));
        binaryBlobPosition_ += size;
        ++statistics_.binaryBlobCount;
        return Kv3Value::Binary(std::move(bytes));
    }

    [[nodiscard]] Kv3Value ReadArray(const std::size_t depth) {
        const auto length = CurrentBuffer().ReadI32();
        if (length < 0) {
            throw std::runtime_error("KV3 array length is negative");
        }
        auto value = Kv3Value::Array(static_cast<std::size_t>(length));
        ++statistics_.arrayCount;
        for (std::int32_t index = 0; index < length; ++index) {
            value.AddArrayValue(ReadValue(ReadType(), depth + 1));
        }
        return value;
    }

    [[nodiscard]] Kv3Value ReadTypedArray(
        const std::int32_t length,
        const bool useAuxiliary,
        const std::size_t depth) {
        if (length < 0) {
            throw std::runtime_error("KV3 typed array length is negative");
        }
        const auto elementType = ReadType();
        auto value = Kv3Value::Array(static_cast<std::size_t>(length));
        ++statistics_.arrayCount;

        if (useAuxiliary) {
            usingAuxiliary_ = !usingAuxiliary_;
        }
        try {
            for (std::int32_t index = 0; index < length; ++index) {
                value.AddArrayValue(ReadValue(elementType, depth + 1));
            }
        } catch (...) {
            if (useAuxiliary) {
                usingAuxiliary_ = !usingAuxiliary_;
            }
            throw;
        }
        if (useAuxiliary) {
            usingAuxiliary_ = !usingAuxiliary_;
        }
        return value;
    }

    [[nodiscard]] Kv3Value ReadObject(const std::size_t depth) {
        if (objectLengthPosition_ >= payload_.objectLengths.size()) {
            throw std::runtime_error("KV3 object length stream is truncated");
        }
        const auto length = payload_.objectLengths[objectLengthPosition_++];
        if (length < 0) {
            throw std::runtime_error("KV3 object length is negative");
        }

        auto value = Kv3Value::Object(static_cast<std::size_t>(length));
        ++statistics_.objectCount;
        for (std::int32_t index = 0; index < length; ++index) {
            const auto childType = ReadType();
            const auto name = ReadStringById(CurrentBuffer().ReadI32());
            value.AddProperty(name, ReadValue(childType, depth + 1));
        }
        return value;
    }

    [[nodiscard]] std::string ReadStringById(const std::int32_t id) const {
        if (id == -1) {
            return {};
        }
        if (id < 0 || static_cast<std::size_t>(id) >= payload_.strings.size()) {
            throw std::runtime_error("KV3 string id is outside the string table");
        }
        return payload_.strings[static_cast<std::size_t>(id)];
    }

    [[nodiscard]] Kv3Buffer& CurrentBuffer() noexcept {
        return usingAuxiliary_ ? payload_.auxiliaryBuffer : payload_.mainBuffer;
    }

    Kv3Payload payload_;
    const Kv3DecodeLimits& limits_;
    Kv3DecodeStatistics statistics_;
    std::size_t typePosition_ = 0;
    std::size_t objectLengthPosition_ = 0;
    std::size_t binaryBlobLengthPosition_ = 0;
    std::size_t binaryBlobPosition_ = 0;
    bool usingAuxiliary_ = false;
};

} // namespace

Kv3Document BinaryKv3Decoder::Decode(
    const resource::ResourceDocument& document,
    const resource::ResourceBlock& dataBlock,
    const Kv3DecodeLimits& limits) const {
    const auto header = BinaryKv3HeaderReader{}.Read(
        document.file.bytes,
        dataBlock.offset,
        dataBlock.size);
    auto payload = Kv3PayloadDecoder{}.Decode(
        document.file.bytes,
        dataBlock.offset,
        dataBlock.size,
        header,
        limits);
    return ValueDecoder(std::move(payload), limits).Decode();
}

} // namespace vmsourceconv::serialization::kv3
