#include "io/PathUtils.h"

#include <cctype>
#include <stdexcept>

namespace vmsourceconv::io {

std::string SafeFileComponent(const std::string& value) {
    std::string output;
    output.reserve(value.size());

    for (const char rawCharacter : value) {
        const auto character = static_cast<unsigned char>(rawCharacter);
        output.push_back(
            std::isalnum(character) != 0
                    || character == static_cast<unsigned char>('_')
                    || character == static_cast<unsigned char>('-')
                ? static_cast<char>(character)
                : '_');
    }

    return output.empty() ? "UNKNOWN" : output;
}

void EnsureDirectory(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        throw std::runtime_error(
            "unable to create directory '" + directory.string()
            + "': " + error.message());
    }
}

} // namespace vmsourceconv::io
