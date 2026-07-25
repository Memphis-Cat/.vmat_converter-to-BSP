#include "discovery/ResourceIdentity.h"

#include <algorithm>
#include <cctype>
#include <vector>

namespace vmsourceconv::discovery {
namespace {

void SetError(std::string* output, const char* message) {
    if (output != nullptr) {
        *output = message;
    }
}

bool HasCompiledSuffix(const std::string_view value) {
    return value.size() >= 2U
        && value[value.size() - 2U] == '_'
        && value[value.size() - 1U] == 'c';
}

} // namespace

std::optional<ResourceIdentity> ResourceIdentity::TryCreate(
    const std::string_view value,
    std::string* error) {
    if (error != nullptr) {
        error->clear();
    }

    std::string normalized(value);
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    while (normalized.rfind("./", 0) == 0) {
        normalized.erase(0, 2);
    }

    if (normalized.empty() || normalized.front() == '/') {
        SetError(error, "resource path is empty or absolute");
        return std::nullopt;
    }
    if (normalized.size() >= 2U
        && std::isalpha(static_cast<unsigned char>(normalized[0])) != 0
        && normalized[1] == ':') {
        SetError(error, "resource path contains a drive prefix");
        return std::nullopt;
    }

    std::vector<std::string> components;
    std::size_t begin = 0;
    while (begin <= normalized.size()) {
        const auto end = normalized.find('/', begin);
        const auto length = end == std::string::npos
            ? normalized.size() - begin
            : end - begin;
        std::string component = normalized.substr(begin, length);
        if (!component.empty() && component != ".") {
            if (component == "..") {
                SetError(error, "resource path contains parent traversal");
                return std::nullopt;
            }
            std::transform(
                component.begin(),
                component.end(),
                component.begin(),
                [](const unsigned char character) {
                    return static_cast<char>(std::tolower(character));
                });
            components.push_back(std::move(component));
        }
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1U;
    }

    if (components.empty()) {
        SetError(error, "resource path has no usable components");
        return std::nullopt;
    }

    std::string canonical;
    for (std::size_t index = 0; index < components.size(); ++index) {
        if (index != 0U) {
            canonical.push_back('/');
        }
        canonical += components[index];
    }

    if (HasCompiledSuffix(canonical)) {
        canonical.erase(canonical.size() - 2U);
    }
    if (canonical.empty()) {
        SetError(error, "resource path is only a compiled suffix");
        return std::nullopt;
    }

    ResourceIdentity identity;
    identity.logicalName = canonical;
    identity.key = canonical + "_c";
    identity.compiledRelativePath = std::filesystem::path(identity.key);
    return identity;
}

} // namespace vmsourceconv::discovery
