#pragma once

#include "serialization/kv3/Kv3Document.h"

#include <filesystem>
#include <string>
#include <vector>

namespace vmsourceconv::serialization::kv3 {

class Kv3DocumentExporter final {
public:
    [[nodiscard]] static bool Matches(
        const std::string& logicalName,
        const std::vector<std::string>& filters);

    [[nodiscard]] static std::filesystem::path Export(
        const Kv3Document& document,
        const std::string& logicalName,
        const std::filesystem::path& outputDirectory);

    [[nodiscard]] static std::filesystem::path OutputPath(
        const std::string& logicalName,
        const std::filesystem::path& outputDirectory);
};

} // namespace vmsourceconv::serialization::kv3
