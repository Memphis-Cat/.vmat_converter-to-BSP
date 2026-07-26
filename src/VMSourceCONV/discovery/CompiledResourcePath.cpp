#include "discovery/CompiledResourcePath.h"

#include "discovery/ResourceIdentity.h"

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

[[nodiscard]] bool IsContentDirectory(const std::string& value) {
    constexpr std::array<std::string_view, 8> names{
        "maps", "materials", "models", "particles",
        "panorama", "resource", "scripts", "sounds",
    };
    return std::find(names.begin(), names.end(), value) != names.end();
}

} // namespace

std::string NormalizeLogicalResourceName(const std::string_view value) {
    const auto identity = ResourceIdentity::TryCreate(value);
    return identity.has_value() ? identity->logicalName : std::string{};
}

std::optional<std::filesystem::path> ToCompiledRelativePath(
    const std::string_view logicalName) {
    const auto identity = ResourceIdentity::TryCreate(logicalName);
    return identity.has_value()
        ? std::optional<std::filesystem::path>(identity->compiledRelativePath)
        : std::nullopt;
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
