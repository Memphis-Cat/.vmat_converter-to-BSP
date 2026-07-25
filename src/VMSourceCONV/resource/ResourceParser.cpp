#include "resource/ResourceParser.h"

#include "core/ParseError.h"
#include "io/BinaryReader.h"
#include "resource/RelativeOffset.h"
#include "resource/ResourceValidator.h"

#include <utility>

namespace vmsourceconv::resource {
namespace {

constexpr std::size_t HeaderSize = 16;
constexpr std::size_t BlockEntrySize = 12;
constexpr std::uint32_t MaximumReasonableBlockCount = 1'000'000;

} // namespace

ResourceDocument ResourceParser::Parse(io::FileData file, std::vector<core::Diagnostic>& diagnostics) const {
    if (file.bytes.size() < HeaderSize) {
        throw core::ParseError("file is too small to contain a Source 2 resource header", 0);
    }

    ResourceDocument document;
    document.file = std::move(file);

    io::BinaryReader reader(document.file.bytes);
    document.header.declaredFileSize = reader.ReadU32();
    document.header.headerVersion = reader.ReadU16();
    document.header.resourceVersion = reader.ReadU16();

    const auto directoryOffsetField = reader.Position();
    document.header.blockDirectoryRelativeOffset = reader.ReadU32();
    document.header.blockCount = reader.ReadU32();
    document.header.blockDirectoryOffset = ResolveRelativeOffset(
        directoryOffsetField,
        document.header.blockDirectoryRelativeOffset);

    if (document.header.blockCount > MaximumReasonableBlockCount) {
        throw core::ParseError("block count is unreasonably large", 12);
    }

    const auto directorySize = CheckedMultiply(
        static_cast<std::size_t>(document.header.blockCount),
        BlockEntrySize);
    const auto directoryEnd = CheckedAdd(document.header.blockDirectoryOffset, directorySize);
    if (directoryEnd > reader.Size()) {
        throw core::ParseError("block directory extends beyond the input file", document.header.blockDirectoryOffset);
    }

    reader.Seek(document.header.blockDirectoryOffset);
    document.blocks.reserve(document.header.blockCount);

    for (std::size_t index = 0; index < document.header.blockCount; ++index) {
        ResourceBlock block;
        block.index = index;
        block.directoryEntryOffset = reader.Position();
        block.type = FourCC(reader.ReadU32());

        const auto relativeOffsetField = reader.Position();
        block.relativeOffset = reader.ReadU32();
        block.offset = ResolveRelativeOffset(relativeOffsetField, block.relativeOffset);
        block.size = reader.ReadU32();

        document.blocks.push_back(block);
    }

    ResourceValidator{}.Validate(document, diagnostics);
    return document;
}

} // namespace vmsourceconv::resource
