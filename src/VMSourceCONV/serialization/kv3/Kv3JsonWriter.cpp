#include "serialization/kv3/Kv3JsonWriter.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string_view>

namespace vmsourceconv::serialization::kv3 {
namespace {

void Indent(std::ostream& output, const std::size_t depth) {
    for (std::size_t index = 0; index < depth * 2U; ++index) {
        output.put(' ');
    }
}

void WriteEscaped(std::ostream& output, const std::string_view value) {
    output.put('"');
    for (const unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20U) {
                    output << "\\u" << std::hex << std::setw(4)
                           << std::setfill('0')
                           << static_cast<unsigned int>(character)
                           << std::dec << std::setfill(' ');
                } else {
                    output.put(static_cast<char>(character));
                }
                break;
        }
    }
    output.put('"');
}

void WriteHexBlob(
    const std::vector<std::uint8_t>& bytes,
    std::ostream& output) {
    static constexpr char Digits[] = "0123456789abcdef";
    output << "{\"$binaryHex\":\"";
    for (const auto byte : bytes) {
        output.put(Digits[(byte >> 4U) & 0x0FU]);
        output.put(Digits[byte & 0x0FU]);
    }
    output << "\"}";
}

void WriteValueRecursive(
    const Kv3Value& value,
    std::ostream& output,
    const std::size_t depth) {
    switch (value.Type()) {
        case Kv3ValueType::Null:
            output << "null";
            return;
        case Kv3ValueType::Boolean:
            output << (value.BooleanValue() ? "true" : "false");
            return;
        case Kv3ValueType::SignedInteger:
            output << value.SignedValue();
            return;
        case Kv3ValueType::UnsignedInteger:
            output << value.UnsignedValue();
            return;
        case Kv3ValueType::FloatingPoint: {
            const auto number = value.FloatingValue();
            if (std::isfinite(number)) {
                output << std::setprecision(std::numeric_limits<double>::max_digits10)
                       << number;
            } else {
                WriteEscaped(output, std::isnan(number)
                    ? "NaN"
                    : (number < 0.0 ? "-Infinity" : "Infinity"));
            }
            return;
        }
        case Kv3ValueType::String:
            WriteEscaped(output, value.StringValue());
            return;
        case Kv3ValueType::BinaryBlob:
            WriteHexBlob(value.BinaryValue(), output);
            return;
        case Kv3ValueType::Array: {
            const auto& values = value.ArrayValues();
            output.put('[');
            for (std::size_t index = 0; index < values.size(); ++index) {
                output << (index == 0 ? "\n" : ",\n");
                Indent(output, depth + 1);
                WriteValueRecursive(values[index], output, depth + 1);
            }
            if (!values.empty()) {
                output.put('\n');
                Indent(output, depth);
            }
            output.put(']');
            return;
        }
        case Kv3ValueType::Object: {
            const auto& keys = value.ObjectKeys();
            const auto& values = value.ObjectValues();
            output.put('{');
            for (std::size_t index = 0; index < values.size(); ++index) {
                output << (index == 0 ? "\n" : ",\n");
                Indent(output, depth + 1);
                WriteEscaped(output, keys[index]);
                output << ": ";
                WriteValueRecursive(values[index], output, depth + 1);
            }
            if (!values.empty()) {
                output.put('\n');
                Indent(output, depth);
            }
            output.put('}');
            return;
        }
    }
}

} // namespace

void Kv3JsonWriter::Write(
    const Kv3Document& document,
    std::ostream& output) const {
    WriteValueRecursive(document.root, output, 0);
    output.put('\n');
}

void Kv3JsonWriter::WriteValue(
    const Kv3Value& value,
    std::ostream& output) const {
    WriteValueRecursive(value, output, 0);
    output.put('\n');
}

void Kv3JsonWriter::WriteFile(
    const Kv3Document& document,
    const std::filesystem::path& path) const {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("unable to open KV3 JSON file: " + path.string());
    }
    Write(document, stream);
    if (!stream) {
        throw std::runtime_error("unable to write KV3 JSON file: " + path.string());
    }
}

} // namespace vmsourceconv::serialization::kv3
