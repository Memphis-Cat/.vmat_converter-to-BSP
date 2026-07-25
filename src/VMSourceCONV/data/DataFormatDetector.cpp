#include "data/DataFormatDetector.h"

#include "data/HexPreview.h"
#include "serialization/kv3/BinaryKv3HeaderReader.h"
#include "serialization/kv3/BinaryKv3Magic.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace vmsourceconv::data {
namespace {

std::uint32_t ReadU32(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

bool StartsWithTextKv3(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::size_t size) {
    std::size_t position = offset;
    const auto end = offset + size;

    if (size >= 3
        && bytes[position] == 0xEFU
        && bytes[position + 1] == 0xBBU
        && bytes[position + 2] == 0xBFU) {
        position += 3;
    }

    while (position < end
        && std::isspace(static_cast<unsigned char>(bytes[position])) != 0) {
        ++position;
    }

    constexpr std::string_view signature = "<!-- kv3";
    if (static_cast<std::size_t>(end - position) < signature.size()) {
        return false;
    }

    return std::equal(
        signature.begin(),
        signature.end(),
        bytes.begin() + static_cast<std::ptrdiff_t>(position),
        [](const char expected, const std::uint8_t actual) {
            return static_cast<std::uint8_t>(expected) == actual;
        });
}

} // namespace

DataInspectionResult DataFormatDetector::Inspect(
    const std::vector<std::uint8_t>& bytes,
    const resource::ResourceBlock& block,
    const bool hasIntrospectionBlock) const {
    DataInspectionResult result;
    result.blockIndex = block.index;
    result.offset = block.offset;
    result.size = block.size;

    if (block.offset > bytes.size()
        || static_cast<std::size_t>(block.size) > bytes.size() - block.offset) {
        result.format = DataFormat::Opaque;
        result.notes.emplace_back("DATA block range is outside the resource");
        return result;
    }

    result.prefixHex = MakeHexPreview(bytes, block.offset, block.size);

    if (block.size == 0) {
        result.format = DataFormat::Empty;
        return result;
    }

    if (block.size >= 4) {
        const auto magic = ReadU32(bytes, block.offset);
        if (serialization::kv3::IsBinaryKv3Magic(magic)) {
            result.format = DataFormat::BinaryKv3;
            try {
                result.binaryKv3 =
                    serialization::kv3::BinaryKv3HeaderReader{}.Read(
                        bytes,
                        block.offset,
                        block.size);
                for (const auto& warning : result.binaryKv3->warnings) {
                    result.notes.push_back(warning);
                }
            } catch (const std::runtime_error& error) {
                result.notes.emplace_back(error.what());
            }
            return result;
        }
    }

    if (StartsWithTextKv3(bytes, block.offset, block.size)) {
        result.format = DataFormat::TextKv3;
        return result;
    }

    result.format = hasIntrospectionBlock
        ? DataFormat::Introspection
        : DataFormat::Opaque;
    return result;
}

} // namespace vmsourceconv::data
