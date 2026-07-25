#include "vpk/VpkArchive.h"

#include "vpk/Crc32.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace vmsourceconv::vpk {
namespace {

constexpr std::uint32_t VpkMagic = 0x55AA1234U;
constexpr std::uint16_t DirectoryArchiveIndex = 0x7FFFU;
constexpr std::size_t MaximumTreeStringLength = 1024U * 1024U;
constexpr std::uint64_t ArchiveMd5RecordSize = 28U;

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
            throw std::runtime_error("VPK tree string is unreasonably large");
        }
    }
}

void ReadExact(
    std::istream& stream,
    std::uint8_t* output,
    const std::size_t size) {
    if (size == 0U) {
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

std::uint64_t CheckedAdd(
    const std::uint64_t left,
    const std::uint64_t right,
    const char* message) {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        throw std::runtime_error(message);
    }
    return left + right;
}

std::uint64_t GetFileSize(const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) {
        throw std::runtime_error(
            "unable to determine file size: " + path.string());
    }
    return static_cast<std::uint64_t>(size);
}

std::string Hex32(const std::uint32_t value) {
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex
           << std::setw(8) << std::setfill('0') << value;
    return output.str();
}

bool EndsWithIgnoreCase(
    const std::string& value,
    const std::string& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return std::equal(
        suffix.rbegin(), suffix.rend(), value.rbegin(),
        [](const unsigned char left, const unsigned char right) {
            return std::tolower(left) == std::tolower(right);
        });
}

std::string PackageStem(const std::filesystem::path& path) {
    auto stem = path.stem().string();
    constexpr std::string_view suffix = "_dir";
    if (stem.size() >= suffix.size()
        && stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) == 0) {
        stem.erase(stem.size() - suffix.size());
    }
    return stem;
}

std::optional<std::uint16_t> ParseChunkIndex(
    const std::filesystem::path& path,
    const std::string& packageStem) {
    const auto filename = path.filename().string();
    const auto prefix = packageStem + "_";
    if (filename.size() != prefix.size() + 7U
        || filename.compare(0, prefix.size(), prefix) != 0
        || !EndsWithIgnoreCase(filename, ".vpk")) {
        return std::nullopt;
    }
    const auto digits = filename.substr(prefix.size(), 3U);
    if (!std::all_of(digits.begin(), digits.end(), [](const unsigned char c) {
            return std::isdigit(c) != 0;
        })) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(
        static_cast<unsigned int>(digits[0] - '0') * 100U
        + static_cast<unsigned int>(digits[1] - '0') * 10U
        + static_cast<unsigned int>(digits[2] - '0'));
}

void AddIssue(
    VpkVerificationReport& report,
    const VpkVerificationSeverity severity,
    std::string code,
    std::string message) {
    report.issues.push_back({severity, std::move(code), std::move(message)});
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

const VpkV2Sections& VpkArchive::Sections() const noexcept {
    return sections_;
}

const std::unordered_map<std::string, VpkEntry>&
VpkArchive::Entries() const noexcept {
    return entries_;
}

const std::vector<std::string>& VpkArchive::DuplicatePaths() const noexcept {
    return duplicatePaths_;
}

const VpkEntry* VpkArchive::Find(const std::string_view path) const {
    const auto iterator = entries_.find(NormalizePath(path));
    return iterator == entries_.end() ? nullptr : &iterator->second;
}
