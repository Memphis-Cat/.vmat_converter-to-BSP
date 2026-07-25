#include "vpk/VpkArchive.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace vmsourceconv::vpk {
namespace {

constexpr std::uint32_t VpkMagic = 0x55AA1234U;
constexpr std::uint16_t DirectoryArchiveIndex = 0x7FFFU;
constexpr std::size_t MaximumTreeStringLength = 1024U * 1024U;

std::uint16_t ReadU16(std::istream& stream) {
    std::array<unsigned char, 2> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()), 2);
    if (!stream) {
        throw std::runtime_error("unexpected end of VPK");
    }

    return static_cast<std::uint16_t>(bytes[0])
        | static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(bytes[1]) << 8U);
}

std::uint32_t ReadU32(std::istream& stream) {
    std::array<unsigned char, 4> bytes{};
    stream.read(reinterpret_cast<char*>(bytes.data()), 4);
    if (!stream) {
        throw std::runtime_error("unexpected end of VPK");
    }

    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

std::string ReadCString(
    std::istream& stream,
    const std::streampos treeEnd) {
    std::string value;

    while (true) {
        const auto position = stream.tellg();
        if (position == std::streampos(-1) || position >= treeEnd) {
            throw std::runtime_error("unterminated VPK tree string");
        }

        char character = 0;
        stream.get(character);
        if (!stream) {
            throw std::runtime_error("unexpected end of VPK tree");
        }

        if (character == '\0') {
            return value;
        }

        value.push_back(character);
        if (value.size() > MaximumTreeStringLength) {
            throw std::runtime_error(
                "VPK tree string is unreasonably large");
        }
    }
}

void ReadExact(
    std::istream& stream,
    std::uint8_t* output,
    const std::size_t size) {
    if (size == 0) {
        return;
    }

    if (size > static_cast<std::size_t>(
            std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("VPK read is too large");
    }

    stream.read(
        reinterpret_cast<char*>(output),
        static_cast<std::streamsize>(size));
    if (!stream) {
        throw std::runtime_error("unexpected end of VPK entry data");
    }
}

} // namespace

VpkArchive::VpkArchive(const std::filesystem::path& directoryFile)
    : directoryFile_(
          std::filesystem::absolute(directoryFile).lexically_normal()) {
    Parse();
}

const std::filesystem::path& VpkArchive::Path() const noexcept {
    return directoryFile_;
}

std::uint32_t VpkArchive::Version() const noexcept {
    return version_;
}

std::size_t VpkArchive::EntryCount() const noexcept {
    return entries_.size();
}

const VpkEntry* VpkArchive::Find(const std::string_view path) const {
    const auto iterator = entries_.find(NormalizePath(path));
    return iterator == entries_.end() ? nullptr : &iterator->second;
}

io::FileData VpkArchive::Read(const VpkEntry& entry) const {
    io::FileData data;
    data.path = std::filesystem::path(
        directoryFile_.string() + "::" + entry.path);

    const auto totalLength =
        static_cast<std::uint64_t>(entry.preloadBytes.size())
        + static_cast<std::uint64_t>(entry.length);
    if (totalLength > static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("VPK entry is too large");
    }

    data.bytes.resize(static_cast<std::size_t>(totalLength));
    std::copy(
        entry.preloadBytes.begin(),
        entry.preloadBytes.end(),
        data.bytes.begin());

    if (entry.length == 0) {
        return data;
    }

    const auto sourcePath =
        entry.archiveIndex == DirectoryArchiveIndex
            ? directoryFile_
            : ChunkPath(entry.archiveIndex);

    std::ifstream stream(sourcePath, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error(
            "unable to open VPK data file: " + sourcePath.string());
    }

    const auto sourceSizePosition = stream.tellg();
    if (sourceSizePosition == std::streampos(-1)) {
        throw std::runtime_error("unable to determine VPK data size");
    }

    const auto sourceSize = static_cast<std::uint64_t>(sourceSizePosition);
    const std::uint64_t baseOffset =
        entry.archiveIndex == DirectoryArchiveIndex
            ? static_cast<std::uint64_t>(headerSize_) + treeSize_
            : 0U;
    const auto absoluteOffset = baseOffset + entry.offset;
    const auto endOffset = absoluteOffset + entry.length;

    if (absoluteOffset > sourceSize || endOffset > sourceSize) {
        throw std::runtime_error("VPK entry range is outside its data file");
    }

    stream.seekg(
        static_cast<std::streamoff>(absoluteOffset),
        std::ios::beg);
    if (!stream) {
        throw std::runtime_error("unable to seek VPK entry data");
    }

    ReadExact(
        stream,
        data.bytes.data() + entry.preloadBytes.size(),
        entry.length);
    return data;
}

void VpkArchive::Parse() {
    std::ifstream stream(directoryFile_, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error(
            "unable to open VPK: " + directoryFile_.string());
    }

    const auto fileSizePosition = stream.tellg();
    if (fileSizePosition == std::streampos(-1)
        || fileSizePosition < std::streampos(12)) {
        throw std::runtime_error(
            "file is too small to be a VPK: " + directoryFile_.string());
    }

    const auto fileSize = static_cast<std::uint64_t>(fileSizePosition);
    stream.seekg(0, std::ios::beg);

    if (ReadU32(stream) != VpkMagic) {
        throw std::runtime_error(
            "file is not a Valve VPK: " + directoryFile_.string());
    }

    version_ = ReadU32(stream);
    treeSize_ = ReadU32(stream);

    if (version_ == 1U) {
        headerSize_ = 12U;
    } else if (version_ == 2U) {
        (void)ReadU32(stream);
        (void)ReadU32(stream);
        (void)ReadU32(stream);
        (void)ReadU32(stream);
        headerSize_ = 28U;
    } else {
        throw std::runtime_error(
            "unsupported VPK version " + std::to_string(version_)
            + ": " + directoryFile_.string());
    }

    const auto treeEndValue =
        static_cast<std::uint64_t>(headerSize_) + treeSize_;
    if (treeEndValue > fileSize
        || treeEndValue > static_cast<std::uint64_t>(
            std::numeric_limits<std::streamoff>::max())) {
        throw std::runtime_error("VPK directory tree is outside the file");
    }

    const auto treeEnd = std::streampos(
        static_cast<std::streamoff>(treeEndValue));

    while (true) {
        const auto extension = ReadCString(stream, treeEnd);
        if (extension.empty()) {
            break;
        }

        while (true) {
            const auto directory = ReadCString(stream, treeEnd);
            if (directory.empty()) {
                break;
            }

            while (true) {
                const auto filename = ReadCString(stream, treeEnd);
                if (filename.empty()) {
                    break;
                }

                VpkEntry entry;
                entry.crc32 = ReadU32(stream);
                const auto preloadSize = ReadU16(stream);
                entry.archiveIndex = ReadU16(stream);
                entry.offset = ReadU32(stream);
                entry.length = ReadU32(stream);

                if (ReadU16(stream) != 0xFFFFU) {
                    throw std::runtime_error(
                        "invalid VPK entry terminator");
                }

                entry.preloadBytes.resize(preloadSize);
                ReadExact(
                    stream,
                    entry.preloadBytes.data(),
                    entry.preloadBytes.size());

                std::string fullPath;
                if (directory != " ") {
                    fullPath = directory;
                    fullPath.push_back('/');
                }

                fullPath += filename;
                if (extension != " ") {
                    fullPath.push_back('.');
                    fullPath += extension;
                }

                entry.path = NormalizePath(fullPath);
                entries_.insert_or_assign(entry.path, std::move(entry));
            }
        }
    }
}

std::filesystem::path VpkArchive::ChunkPath(
    const std::uint16_t archiveIndex) const {
    auto stem = directoryFile_.stem().string();
    constexpr std::string_view directorySuffix = "_dir";

    if (stem.size() >= directorySuffix.size()
        && stem.compare(
               stem.size() - directorySuffix.size(),
               directorySuffix.size(),
               directorySuffix)
            == 0) {
        stem.erase(stem.size() - directorySuffix.size());
    }

    std::ostringstream filename;
    filename << stem << '_'
             << std::setw(3) << std::setfill('0') << archiveIndex
             << ".vpk";
    return directoryFile_.parent_path() / filename.str();
}

std::string VpkArchive::NormalizePath(const std::string_view path) {
    std::string normalized;
    normalized.reserve(path.size());

    bool leading = true;
    for (const unsigned char character : path) {
        const auto separatorNormalized =
            character == '\\' ? '/' : static_cast<char>(character);

        if (leading && separatorNormalized == '/') {
            continue;
        }
        leading = false;

        normalized.push_back(static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(separatorNormalized))));
    }

    return normalized;
}

} // namespace vmsourceconv::vpk
