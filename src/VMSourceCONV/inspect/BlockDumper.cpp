#include "inspect/BlockDumper.h"

#include "io/PathUtils.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace vmsourceconv::inspect {

void BlockDumper::Dump(
    const resource::ResourceDocument& document,
    const std::filesystem::path& directory) const {
    io::EnsureDirectory(directory);

    for (const auto& block : document.blocks) {
        const auto end = block.EndOffset();
        if (end > document.file.bytes.size()) {
            continue;
        }

        std::ostringstream name;
        name << std::setw(4) << std::setfill('0') << block.index << '_'
             << io::SafeFileComponent(block.type.ToString()) << "_0x"
             << std::uppercase << std::hex << block.offset << std::dec << ".bin";

        const auto path = directory / name.str();
        std::ofstream stream(path, std::ios::binary);
        if (!stream) {
            throw std::runtime_error("unable to create block dump: " + path.string());
        }

        if (block.size != 0) {
            stream.write(
                reinterpret_cast<const char*>(document.file.bytes.data() + block.offset),
                static_cast<std::streamsize>(block.size));
        }

        if (!stream) {
            throw std::runtime_error("unable to write block dump: " + path.string());
        }
    }
}

} // namespace vmsourceconv::inspect
