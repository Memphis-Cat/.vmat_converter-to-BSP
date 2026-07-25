#include "serialization/kv3/Kv3DocumentExporter.h"

#include "serialization/kv3/Kv3JsonWriter.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>

namespace vmsourceconv::serialization::kv3 {
namespace {

[[nodiscard]] std::string LowerCopy(const std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        result.push_back(static_cast<char>(std::tolower(character)));
    }
    return result;
}

[[nodiscard]] std::string SafeFileName(const std::string_view logicalName) {
    std::string result;
    result.reserve(logicalName.size() + 5U);
    bool previousUnderscore = false;
    for (const unsigned char character : logicalName) {
        const auto keep = std::isalnum(character) != 0
            || character == '.' || character == '-' || character == '_';
        const auto output = keep ? static_cast<char>(character) : '_';
        if (output == '_' && previousUnderscore) {
            continue;
        }
        result.push_back(output);
        previousUnderscore = output == '_';
    }
    while (!result.empty() && result.front() == '_') {
        result.erase(result.begin());
    }
    while (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    if (result.empty()) {
        result = "resource";
    }
    result += ".json";
    return result;
}

} // namespace

bool Kv3DocumentExporter::Matches(
    const std::string& logicalName,
    const std::vector<std::string>& filters) {
    if (filters.empty()) {
        return true;
    }

    const auto lowerName = LowerCopy(logicalName);
    return std::any_of(
        filters.begin(),
        filters.end(),
        [&lowerName](const std::string& filter) {
            return lowerName.find(LowerCopy(filter)) != std::string::npos;
        });
}

std::filesystem::path Kv3DocumentExporter::OutputPath(
    const std::string& logicalName,
    const std::filesystem::path& outputDirectory) {
    return outputDirectory / SafeFileName(logicalName);
}

std::filesystem::path Kv3DocumentExporter::Export(
    const Kv3Document& document,
    const std::string& logicalName,
    const std::filesystem::path& outputDirectory) {
    if (outputDirectory.empty()) {
        throw std::runtime_error("KV3 JSON output directory is empty");
    }
    std::filesystem::create_directories(outputDirectory);
    const auto path = OutputPath(logicalName, outputDirectory);
    Kv3JsonWriter{}.WriteFile(document, path);
    return path;
}

} // namespace vmsourceconv::serialization::kv3
