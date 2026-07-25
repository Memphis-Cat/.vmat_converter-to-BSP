#include "inspect/data/DataFormatDetector.h"

#include <algorithm>
#include <limits>

namespace vmsourceconv::inspect::data {
namespace {

constexpr std::uint32_t LegacyBinaryKv3Magic = 0x03564B56U;
constexpr std::uint32_t BinaryKv3Version1Magic = 0x4B563301U;
constexpr std::uint32_t BinaryKv3Version5Magic = 0x4B563305U;
constexpr std::size_t PreviewSize = 32U;

std::uint32_t ReadU32(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U)
        | (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

} // namespace

DataInspectionResult DataFormatDetector::Detect(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::uint32_t size) const {
    DataInspectionResult result;
    result.present = true;
    result.offset = offset;
    result.size = size;

    if (size == 0U) {
        result.format = DataFormat::Empty;
        result.message = "DATA block is present but empty";
        return result;
    }

    if (offset > bytes.size()
        || static_cast<std::size_t>(size) > bytes.size() - offset) {
        result.format = DataFormat::Unknown;
        result.message = "DATA block range is outside the resource";
        return result;
    }

    const auto previewLength = std::min(
        PreviewSize,
        static_cast<std::size_t>(size));
    result.preview.assign(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        bytes.begin() + static_cast<std::ptrdiff_t>(offset + previewLength));

    if (size < 4U) {
        result.format = DataFormat::Unknown;
        result.message = "DATA block is too small to contain a format magic";
        return result;
    }

    result.magic = ReadU32(bytes, offset);
    if (result.magic == LegacyBinaryKv3Magic) {
        result.format = DataFormat::LegacyBinaryKv3;
        result.message = "legacy Binary KV3 encoding detected";
        return result;
    }

    if (result.magic >= BinaryKv3Version1Magic
        && result.magic <= BinaryKv3Version5Magic) {
        result.format = DataFormat::BinaryKv3;
        result.kv3Version = result.magic - 0x4B563300U;
        result.message = "Binary KV3 header detected";
        return result;
    }

    result.format = DataFormat::Unknown;
    result.message = "unrecognized DATA serialization";
    return result;
}

} // namespace vmsourceconv::inspect::data
