#pragma once

namespace vmsourceconv::data {

enum class DataFormat {
    Empty,
    BinaryKv3,
    TextKv3,
    Introspection,
    Opaque,
};

[[nodiscard]] const char* ToString(DataFormat format) noexcept;

} // namespace vmsourceconv::data
