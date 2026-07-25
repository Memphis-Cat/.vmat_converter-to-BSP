#include "data/DataFormat.h"

namespace vmsourceconv::data {

const char* ToString(const DataFormat format) noexcept {
    switch (format) {
        case DataFormat::Empty: return "empty";
        case DataFormat::BinaryKv3: return "binary-kv3";
        case DataFormat::TextKv3: return "text-kv3";
        case DataFormat::Introspection: return "introspection-data";
        case DataFormat::Opaque: return "opaque";
    }

    return "opaque";
}

} // namespace vmsourceconv::data
