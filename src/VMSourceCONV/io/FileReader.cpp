#include "io/FileReader.h"

#include <fstream>
#include <limits>
#include <stdexcept>

namespace vmsourceconv::io {

FileData FileReader::ReadAll(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("unable to open input file: " + path.string());
    }

    const auto end = stream.tellg();
    if (end < 0) {
        throw std::runtime_error("unable to determine input file size: " + path.string());
    }

    const auto unsignedSize = static_cast<unsigned long long>(end);
    if (unsignedSize > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("input file is too large for this build: " + path.string());
    }

    FileData result;
    result.path = path;
    result.bytes.resize(static_cast<std::size_t>(unsignedSize));

    stream.seekg(0, std::ios::beg);
    if (!result.bytes.empty()) {
        stream.read(reinterpret_cast<char*>(result.bytes.data()), static_cast<std::streamsize>(result.bytes.size()));
        if (!stream) {
            throw std::runtime_error("unable to read the complete input file: " + path.string());
        }
    }

    return result;
}

} // namespace vmsourceconv::io
