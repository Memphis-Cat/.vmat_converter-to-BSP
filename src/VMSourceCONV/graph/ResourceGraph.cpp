#include "graph/ResourceGraph.h"

namespace vmsourceconv::graph {

const char* ToString(const ResourceNodeStatus status) noexcept {
    switch (status) {
        case ResourceNodeStatus::Loaded: return "loaded";
        case ResourceNodeStatus::Missing: return "missing";
        case ResourceNodeStatus::ParseError: return "parse-error";
        case ResourceNodeStatus::IoError: return "io-error";
    }
    return "unknown";
}

const char* ToString(const ResourceNodeSource source) noexcept {
    switch (source) {
        case ResourceNodeSource::None: return "none";
        case ResourceNodeSource::LooseFile: return "loose-file";
        case ResourceNodeSource::VpkArchive: return "vpk";
    }
    return "unknown";
}

const char* ToString(const DependencyDisposition disposition) noexcept {
    switch (disposition) {
        case DependencyDisposition::Resolved: return "resolved";
        case DependencyDisposition::Missing: return "missing";
        case DependencyDisposition::Failed: return "failed";
        case DependencyDisposition::Repeated: return "repeated";
        case DependencyDisposition::Cycle: return "cycle";
        case DependencyDisposition::SelfReference: return "self-reference";
        case DependencyDisposition::Skipped: return "skipped";
        case DependencyDisposition::Invalid: return "invalid";
        case DependencyDisposition::DepthLimited: return "depth-limited";
        case DependencyDisposition::ResourceLimited: return "resource-limited";
    }
    return "unknown";
}

} // namespace vmsourceconv::graph
