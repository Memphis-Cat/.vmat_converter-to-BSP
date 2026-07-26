#include "resource/ResourceValidator.h"

#include "resource/ResourceHeader.h"

#include <algorithm>
#include <exception>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vmsourceconv::resource {
namespace {

constexpr std::size_t HeaderSize = 16U;
constexpr std::size_t BlockEntrySize = 12U;

bool Overlaps(
    const std::size_t leftBegin,
    const std::size_t leftEnd,
    const std::size_t rightBegin,
    const std::size_t rightEnd) {
    return leftBegin < rightEnd && rightBegin < leftEnd;
}

bool IsSingletonBlock(const std::string& type) {
    static const std::unordered_set<std::string> singletonTypes{
        "DATA", "RERL", "REDI", "RED2", "NTRO", "VBIB",
    };
    return singletonTypes.find(type) != singletonTypes.end();
}

} // namespace

void ResourceValidator::Validate(
    const ResourceDocument& document,
    std::vector<core::Diagnostic>& diagnostics) const {
    const auto actualSize = document.file.bytes.size();

    if (document.header.headerVersion != KnownResourceHeaderVersion()) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "RESOURCE_HEADER_VERSION",
            "expected Source 2 resource header version 12, found "
                + std::to_string(document.header.headerVersion),
            4,
        });
    }
    if (document.header.declaredFileSize != actualSize) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "DECLARED_FILE_SIZE",
            "header declares "
                + std::to_string(document.header.declaredFileSize)
                + " bytes, actual file contains "
                + std::to_string(actualSize) + " bytes",
            0,
        });
    }

    const auto directoryBegin = document.header.blockDirectoryOffset;
    const auto directorySize =
        static_cast<std::size_t>(document.header.blockCount) * BlockEntrySize;
    const auto directoryEnd = directoryBegin + directorySize;
    if (directorySize != 0U && directoryBegin < HeaderSize) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Error,
            "BLOCK_DIRECTORY_OVERLAPS_HEADER",
            "block directory begins inside the 16-byte resource header",
            directoryBegin,
        });
    }
    if (directoryBegin % 4U != 0U) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Warning,
            "BLOCK_DIRECTORY_ALIGNMENT",
            "block directory is not 4-byte aligned",
            directoryBegin,
        });
    }

    std::vector<const ResourceBlock*> validBlocks;
    validBlocks.reserve(document.blocks.size());
    std::unordered_map<std::string, std::size_t> typeCounts;
    std::size_t maximumOccupiedEnd = std::max(HeaderSize, directoryEnd);

    for (const auto& block : document.blocks) {
        const auto type = block.type.ToString();
        ++typeCounts[type];
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
                "block " + std::to_string(block.index) + " (" + type
                    + ") extends beyond the input file",
                block.directoryEntryOffset,
            });
            continue;
        }
        if (block.size != 0U && block.offset < HeaderSize) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "BLOCK_OVERLAPS_HEADER",
                "block " + std::to_string(block.index) + " (" + type
                    + ") begins inside the resource header",
                block.directoryEntryOffset,
            });
        }
        if (block.size != 0U
            && Overlaps(block.offset, end, directoryBegin, directoryEnd)) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Error,
                "BLOCK_OVERLAPS_DIRECTORY",
                "block " + std::to_string(block.index) + " (" + type
                    + ") overlaps the block directory",
                block.directoryEntryOffset,
            });
        }
        if (block.offset % 4U != 0U) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "BLOCK_ALIGNMENT",
                "block " + std::to_string(block.index) + " (" + type
                    + ") is not 4-byte aligned",
                block.directoryEntryOffset,
            });
        }
        if (block.size == 0U) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Info,
                "EMPTY_BLOCK",
                "block " + std::to_string(block.index) + " (" + type
                    + ") is empty",
                block.directoryEntryOffset,
            });
        }

        maximumOccupiedEnd = std::max(maximumOccupiedEnd, end);
        validBlocks.push_back(&block);
    }

    for (const auto& count : typeCounts) {
        if (count.second > 1U && IsSingletonBlock(count.first)) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "DUPLICATE_SINGLETON_BLOCK",
                "resource contains " + std::to_string(count.second)
                    + " " + count.first
                    + " blocks; readers deterministically use the first",
                std::nullopt,
            });
        }
    }

    std::sort(
        validBlocks.begin(), validBlocks.end(),
        [](const auto* left, const auto* right) {
            return left->offset < right->offset;
        });
    for (std::size_t index = 1; index < validBlocks.size(); ++index) {
        const auto* previous = validBlocks[index - 1U];
        const auto* current = validBlocks[index];
        if (previous->size != 0U
            && current->size != 0U
            && current->offset < previous->EndOffset()) {
            diagnostics.push_back({
                core::DiagnosticSeverity::Warning,
                "OVERLAPPING_BLOCKS",
                "blocks " + std::to_string(previous->index) + " and "
                    + std::to_string(current->index) + " overlap",
                current->directoryEntryOffset,
            });
        }
    }

    if (maximumOccupiedEnd < actualSize) {
        diagnostics.push_back({
            core::DiagnosticSeverity::Info,
            "TRAILING_RESOURCE_DATA",
            std::to_string(actualSize - maximumOccupiedEnd)
                + " trailing byte(s) follow the final declared region",
            maximumOccupiedEnd,
        });
    }
}

} // namespace vmsourceconv::resource
