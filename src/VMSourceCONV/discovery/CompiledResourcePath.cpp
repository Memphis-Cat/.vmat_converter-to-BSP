#include "discovery/CompiledResourcePath.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>

namespace vmsourceconv::discovery {
namespace {

[[nodiscard]] std::string Lowercase(std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

[[nodiscard]] bool EndsWithCompiledSuffix(const std::string_view value) {
    return value.size() >= 2 && value.substr(value.size() - 2) == "_c";
}

[[nodiscard]] bool IsContentDirectory(const std::string& value) {
    constexpr std::array<std::string_view, 8> names{
        "maps",
        "materials",
        "models",
        "particles",
        "panorama",
        "resource",
        "scripts",
        "sounds",
    };

    return std::find(names.begin(), names.end(), value) != names.end();
}

} // namespace

std::string NormalizeLogicalResourceName(const std::string_view value) {
    std::string normalized(value);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');

    while (normalized.rfind("./", 0) == 0) {
        normalized.erase(0, 2);
    }

    return std::filesystem::path(normalized).lexically_normal().generic_string();
}

std::optional<std::filesystem::path> ToCompiledRelativePath(const std::string_view logicalName) {
    const auto normalized = NormalizeLogicalResourceName(logicalName);
    if (normalized.empty() || normalized == ".") {
        return std::nullopt;
    }

    const std::filesystem::path logicalPath(normalized);
    if (logicalPath.is_absolute() || logicalPath.has_root_name() || logicalPath.has_root_directory()) {
        return std::nullopt;
    }

    for (const auto& component : logicalPath) {
        if (component == "..") {
            return std::nullopt;
        }
    }

    auto compiledName = logicalPath.generic_string();
    if (!EndsWithCompiledSuffix(compiledName)) {
        compiledName += "_c";
    }

    return std::filesystem::path(compiledName);
}

std::filesystem::path InferContentRoot(const std::filesystem::path& input) {
    std::error_code error;
    auto absoluteInput = std::filesystem::absolute(input, error);
    if (error) {
        absoluteInput = input;
    }

    absoluteInput = absoluteInput.lexically_normal();
    const auto parent = absoluteInput.parent_path();
    std::filesystem::path prefix = parent.root_path();

    for (const auto& component : parent.relative_path()) {
        const auto lowered = Lowercase(component.string());
        if (IsContentDirectory(lowered)) {
            return prefix.empty() ? parent : prefix;
        }
        prefix /= component;
    }

    return parent;
}

} // namespace vmsourceconv::discovery
