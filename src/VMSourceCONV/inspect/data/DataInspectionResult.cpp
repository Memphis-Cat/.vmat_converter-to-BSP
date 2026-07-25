#include "inspect/data/DataInspectionResult.h"

namespace vmsourceconv::inspect::data {

const char* ToString(const DataFormat format) noexcept {
    switch (format) {
        case DataFormat::None: return "none";
        case DataFormat::Empty: return "empty";
        case DataFormat::BinaryKv3: return "binary-kv3";
        case DataFormat::LegacyBinaryKv3: return "legacy-binary-kv3";
        case DataFormat::Unknown: return "unknown";
    }

    return "unknown";
}

} // namespace vmsourceconv::inspect::data
