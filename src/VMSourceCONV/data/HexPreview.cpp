#include "data/HexPreview.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace vmsourceconv::data {

std::string MakeHexPreview(
    const std::vector<std::uint8_t>& bytes,
    const std::size_t offset,
    const std::size_t size,
    const std::size_t maximumBytes) {
    if (offset > bytes.size() || size > bytes.size() - offset) {
        return {};
    }

    const auto count = std::min(size, maximumBytes);
    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (std::size_t index = 0; index < count; ++index) {
        if (index != 0) {
            output << ' ';
        }
        output << std::setw(2)
               << static_cast<unsigned int>(bytes[offset + index]);
    }

    return output.str();
}

} // namespace vmsourceconv::data
