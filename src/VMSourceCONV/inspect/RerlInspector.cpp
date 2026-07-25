#include "inspect/RerlInspector.h"

#include "core/ParseError.h"
#include "io/BinaryReader.h"
#include "resource/RelativeOffset.h"

#include <string>
#include <utility>

namespace vmsourceconv::inspect {
namespace {

constexpr std::size_t RerlHeaderSize = 8;
constexpr std::size_t RerlEntrySize = 16;
constexpr std::uint32_t MaximumReasonableReferences = 2'000'000;

} // namespace

std::vector<ExternalReference> RerlInspector::Inspect(
    const resource::ResourceDocument& document,
    std::vector<core::Diagnostic>& diagnostics) const {
    std::vector<ExternalReference> references;
    io::BinaryReader reader(document.file.bytes);

    for (const auto& block : document.blocks) {
        if (!block.type.Is("RERL") || block.size == 0) {
            continue;
        }

        try {
            const auto blockEnd = block.EndOffset();
            if (blockEnd > reader.Size() || block.size < RerlHeaderSize) {
                throw core::ParseError("RERL block is truncated", block.offset);
            }

            reader.Seek(block.offset);
            const auto listRelativeOffset = reader.ReadU32();
            const auto referenceCount = reader.ReadU32();

            if (referenceCount > MaximumReasonableReferences) {
                throw core::ParseError("RERL reference count is unreasonably large", block.offset + 4);
            }

            if (referenceCount == 0) {
                continue;
            }

            const auto listOffset = resource::ResolveRelativeOffset(block.offset, listRelativeOffset);
            const auto tableSize = resource::CheckedMultiply(
                static_cast<std::size_t>(referenceCount), RerlEntrySize);
            const auto listEnd = resource::CheckedAdd(listOffset, tableSize);
            if (listOffset < block.offset || listEnd > blockEnd) {
                throw core::ParseError("RERL entry table is outside its block", listOffset);
            }

            references.reserve(references.size() + referenceCount);
            for (std::size_t index = 0; index < referenceCount; ++index) {
                const auto entryOffset = resource::CheckedAdd(
                    listOffset,
                    resource::CheckedMultiply(index, RerlEntrySize));
                reader.Seek(entryOffset);

                ExternalReference reference;
                reference.entryOffset = entryOffset;
                reference.id = reader.ReadU64();

                const auto nameOffsetField = reader.Position();
                const auto nameRelativeOffset = reader.ReadI64();
                reference.stringOffset = resource::ResolveSignedRelativeOffset(
                    nameOffsetField,
                    nameRelativeOffset);

                if (reference.stringOffset < block.offset || reference.stringOffset >= blockEnd) {
                    throw core::ParseError("RERL string points outside its block", nameOffsetField);
                }

                reader.Seek(reference.stringOffset);
                reference.name = reader.ReadCString(blockEnd);
                references.push_back(std::move(reference));
            }
        } catch (const core::ParseError& error) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "RERL_PARSE_ERROR",
                error.what(),
                error.Offset(),
            });
        }
    }

    return references;
}

} // namespace vmsourceconv::inspect
