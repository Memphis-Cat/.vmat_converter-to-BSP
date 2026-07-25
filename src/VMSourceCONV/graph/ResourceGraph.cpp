#include "graph/ResourceGraph.h"

namespace vmsourceconv::graph {

const char* ToString(const ResourceNodeStatus status) noexcept {
    switch (status) {
        case ResourceNodeStatus::Loaded:
            return "loaded";
        case ResourceNodeStatus::Missing:
            return "missing";
        case ResourceNodeStatus::ParseError:
            return "parse-error";
        case ResourceNodeStatus::IoError:
            return "io-error";
    }

    return "unknown";
}

} // namespace vmsourceconv::graph
