#include "resource/ResourceValidator.h"

#include "resource/ResourceHeader.h"

#include <algorithm>
#include <exception>
#include <string>
#include <vector>

namespace vmsourceconv::resource {

void ResourceValidator::Validate(
    const ResourceDocument& document,
    std::vector<core::Diagnostic>& diagnostics) const {
    const auto actualSize = document.file.bytes.size();

    if (document.header.headerVersion != KnownResourceHeaderVersion()) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "RESOURCE_HEADER_VERSION",
            "expected Source 2 resource header version 12, found " +
                std::to_string(document.header.headerVersion),
            4,
        });
    }

    if (document.header.declaredFileSize != actualSize) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "DECLARED_FILE_SIZE",
            "header declares " + std::to_string(document.header.declaredFileSize) +
                " bytes, actual file contains " + std::to_string(actualSize) + " bytes",
            0,
        });
    }

    std::vector<const ResourceBlock*> validBlocks;
    validBlocks.reserve(document.blocks.size());

    for (const auto& block : document.blocks) {
        bool valid = true;
        std::size_t end = block.offset;

        try {
            end = block.EndOffset();
        } catch (const std::exception&) {
            valid = false;
        }

        if (!valid || block.offset > actualSize || end > actualSize) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "BLOCK_OUT_OF_RANGE",
                "block " + std::to_string(block.index) + " (" + block.type.ToString() +
                    ") extends beyond the input file",
                block.directoryEntryOffset,
            });
            continue;
        }

        if (block.size == 0) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Info,
                "EMPTY_BLOCK",
                "block " + std::to_string(block.index) + " (" + block.type.ToString() + ") is empty",
                block.directoryEntryOffset,
            });
        }

        validBlocks.push_back(&block);
    }

    std::sort(validBlocks.begin(), validBlocks.end(), [](const auto* left, const auto* right) {
        return left->offset < right->offset;
    });

    for (std::size_t index = 1; index < validBlocks.size(); ++index) {
        const auto* previous = validBlocks[index - 1];
        const auto* current = validBlocks[index];
        if (previous->size != 0 && current->size != 0 && current->offset < previous->EndOffset()) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "OVERLAPPING_BLOCKS",
                "blocks " + std::to_string(previous->index) + " and " +
                    std::to_string(current->index) + " overlap",
                current->directoryEntryOffset,
            });
        }
    }
}

} // namespace vmsourceconv::resource
