#include "io/FileReader.h"

#include <fstream>
#include <limits>
#include <stdexcept>

namespace vmsourceconv::io {
namespace {

std::uint64_t StreamSize(
    std::ifstream& stream,
    const std::filesystem::path& path) {
    const auto end = stream.tellg();
    if (end < 0) {
        throw std::runtime_error(
            "unable to determine input file size: " + path.string());
    }
    return static_cast<std::uint64_t>(end);
}

void ValidateAllocation(
    const std::uint64_t size,
    const std::uint64_t maximumBytes,
    const std::filesystem::path& path) {
    if (size > maximumBytes) {
        throw std::runtime_error(
            "input exceeds the configured size limit: " + path.string());
    }
    if (size > static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())
        || size > static_cast<std::uint64_t>(
            std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error(
            "input is too large for this build: " + path.string());
    }
}

} // namespace

FileData FileReader::ReadAll(
    const std::filesystem::path& path,
    const std::uint64_t maximumBytes) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("unable to open input file: " + path.string());
    }
    const auto size = StreamSize(stream, path);
    ValidateAllocation(size, maximumBytes, path);
    stream.seekg(0, std::ios::beg);

    FileData result;
    result.path = path;
    result.bytes.resize(static_cast<std::size_t>(size));
    if (!result.bytes.empty()) {
        stream.read(
            reinterpret_cast<char*>(result.bytes.data()),
            static_cast<std::streamsize>(result.bytes.size()));
        if (!stream) {
            throw std::runtime_error(
                "unable to read the complete input file: " + path.string());
        }
    }
    return result;
}

FileData FileReader::ReadRange(
    const std::filesystem::path& path,
    const std::uint64_t offset,
    const std::uint64_t length,
    const std::uint64_t maximumBytes) {
    ValidateAllocation(length, maximumBytes, path);
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("unable to open input file: " + path.string());
    }
    const auto size = StreamSize(stream, path);
    if (offset > size || length > size - offset) {
        throw std::runtime_error(
            "requested input range is outside the file: " + path.string());
    }
    stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error(
            "unable to seek input file: " + path.string());
    }

    FileData result;
    result.path = path;
    result.bytes.resize(static_cast<std::size_t>(length));
    if (!result.bytes.empty()) {
        stream.read(
            reinterpret_cast<char*>(result.bytes.data()),
            static_cast<std::streamsize>(result.bytes.size()));
        if (!stream) {
            throw std::runtime_error(
                "unable to read input range: " + path.string());
        }
    }
    return result;
}

} // namespace vmsourceconv::io
