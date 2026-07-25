#include "cli/CommandLine.h"
#include "core/Diagnostic.h"
#include "core/LogSession.h"
#include "core/ParseError.h"
#include "discovery/ResourceIdentity.h"
#include "discovery/ResourceLocator.h"
#include "graph/ResourceGraphBuilder.h"
#include "inspect/RerlInspector.h"
#include "io/FileReader.h"
#include "resource/ResourceParser.h"
#include "vpk/Crc32.h"
#include "vpk/VpkArchive.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class TestFailure final : public std::runtime_error {
public:
    explicit TestFailure(const std::string& message)
        : std::runtime_error(message) {
    }
};

void Check(
    const bool condition,
    const char* expression,
    const char* file,
    const int line) {
    if (!condition) {
        throw TestFailure(
            std::string(file) + ':' + std::to_string(line)
            + ": check failed: " + expression);
    }
}

#define CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        const auto timestamp = std::chrono::steady_clock::now()
            .time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path()
            / ("vmsourceconv-foundation-" + std::to_string(timestamp));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;

    [[nodiscard]] const std::filesystem::path& Path() const noexcept {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void AppendU16(std::vector<std::uint8_t>& bytes, const std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void AppendU32(std::vector<std::uint8_t>& bytes, const std::uint32_t value) {
    for (unsigned int shift = 0; shift < 32U; shift += 8U) {
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

void PatchU16(
    std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::uint16_t value) {
    CHECK(offset + 2U <= bytes.size());
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void PatchU32(
    std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::uint32_t value) {
    CHECK(offset + 4U <= bytes.size());
    for (unsigned int index = 0; index < 4U; ++index) {
        bytes[offset + index] = static_cast<std::uint8_t>(
            (value >> (index * 8U)) & 0xFFU);
    }
}

void PatchU64(
    std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::uint64_t value) {
    CHECK(offset + 8U <= bytes.size());
    for (unsigned int index = 0; index < 8U; ++index) {
        bytes[offset + index] = static_cast<std::uint8_t>(
            (value >> (index * 8U)) & 0xFFU);
    }
}

void PatchI64(
    std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::int64_t value) {
    PatchU64(bytes, offset, static_cast<std::uint64_t>(value));
}

void AppendCString(std::vector<std::uint8_t>& bytes, const std::string& value) {
    bytes.insert(bytes.end(), value.begin(), value.end());
    bytes.push_back(0U);
}

std::uint32_t FourCc(const std::string& value) {
    CHECK(value.size() == 4U);
    return static_cast<std::uint32_t>(static_cast<unsigned char>(value[0]))
        | (static_cast<std::uint32_t>(
               static_cast<unsigned char>(value[1])) << 8U)
        | (static_cast<std::uint32_t>(
               static_cast<unsigned char>(value[2])) << 16U)
        | (static_cast<std::uint32_t>(
               static_cast<unsigned char>(value[3])) << 24U);
}

struct BlockFixture {
    std::string type;
    std::vector<std::uint8_t> data;
};

std::vector<std::uint8_t> BuildResource(
    const std::vector<BlockFixture>& blocks) {
    constexpr std::size_t HeaderSize = 16U;
    constexpr std::size_t EntrySize = 12U;
    const auto directorySize = blocks.size() * EntrySize;
    const auto firstDataOffset = HeaderSize + directorySize;

    std::vector<std::uint8_t> bytes(firstDataOffset, 0U);
    PatchU16(bytes, 4U, 12U);
    PatchU16(bytes, 6U, 1U);
    PatchU32(bytes, 8U, 8U);
    PatchU32(bytes, 12U, static_cast<std::uint32_t>(blocks.size()));

    auto dataOffset = firstDataOffset;
    for (std::size_t index = 0; index < blocks.size(); ++index) {
        const auto entryOffset = HeaderSize + index * EntrySize;
        PatchU32(bytes, entryOffset, FourCc(blocks[index].type));
        const auto relativeField = entryOffset + 4U;
        CHECK(dataOffset >= relativeField);
        PatchU32(
            bytes,
            relativeField,
            static_cast<std::uint32_t>(dataOffset - relativeField));
        PatchU32(
            bytes,
            entryOffset + 8U,
            static_cast<std::uint32_t>(blocks[index].data.size()));
        bytes.insert(
            bytes.end(),
            blocks[index].data.begin(),
            blocks[index].data.end());
        dataOffset += blocks[index].data.size();
        while (bytes.size() % 4U != 0U) {
            bytes.push_back(0U);
            ++dataOffset;
        }
    }
    PatchU32(bytes, 0U, static_cast<std::uint32_t>(bytes.size()));
    return bytes;
}

std::vector<std::uint8_t> BuildRerl(
    const std::vector<vmsourceconv::inspect::ExternalReference>& references) {
    constexpr std::size_t HeaderSize = 8U;
    constexpr std::size_t EntrySize = 16U;
    std::vector<std::uint8_t> bytes(HeaderSize + references.size() * EntrySize, 0U);
    PatchU32(bytes, 0U, 8U);
    PatchU32(bytes, 4U, static_cast<std::uint32_t>(references.size()));

    for (std::size_t index = 0; index < references.size(); ++index) {
        const auto entryOffset = HeaderSize + index * EntrySize;
        PatchU64(bytes, entryOffset, references[index].id);
        const auto relativeField = entryOffset + 8U;
        const auto stringOffset = bytes.size();
        const auto difference = static_cast<std::int64_t>(stringOffset)
            - static_cast<std::int64_t>(relativeField);
        PatchI64(bytes, relativeField, difference);
        AppendCString(bytes, references[index].name);
    }
    return bytes;
}

std::vector<std::uint8_t> BuildResourceWithRerl(
    const std::vector<vmsourceconv::inspect::ExternalReference>& references) {
    return BuildResource({{"RERL", BuildRerl(references)}});
}

void WriteBytes(
    const std::filesystem::path& path,
    const std::vector<std::uint8_t>& bytes) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    CHECK(static_cast<bool>(output));
    if (!bytes.empty()) {
        output.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    }
    CHECK(static_cast<bool>(output));
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    CHECK(static_cast<bool>(input));
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

bool HasDiagnostic(
    const std::vector<vmsourceconv::core::Diagnostic>& diagnostics,
    const std::string& code) {
    return std::any_of(
        diagnostics.begin(), diagnostics.end(),
        [&code](const vmsourceconv::core::Diagnostic& diagnostic) {
            return diagnostic.code == code;
        });
}
