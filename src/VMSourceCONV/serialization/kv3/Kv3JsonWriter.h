#pragma once

#include "serialization/kv3/Kv3Document.h"
#include "serialization/kv3/Kv3Value.h"

#include <filesystem>
#include <iosfwd>

namespace vmsourceconv::serialization::kv3 {

class Kv3JsonWriter final {
public:
    void Write(const Kv3Document& document, std::ostream& output) const;
    void WriteValue(const Kv3Value& value, std::ostream& output) const;
    void WriteFile(const Kv3Document& document, const std::filesystem::path& path) const;
};

} // namespace vmsourceconv::serialization::kv3
