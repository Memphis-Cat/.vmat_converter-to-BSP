#include "serialization/kv3/BinaryKv3Header.h"

#include <iomanip>
#include <sstream>

namespace vmsourceconv::serialization::kv3 {

std::string BinaryKv3Header::FormatIdString() const {
    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (std::size_t index = 0; index < formatId.size(); ++index) {
        output << std::setw(2) << static_cast<unsigned int>(formatId[index]);
    }

    return output.str();
}

} // namespace vmsourceconv::serialization::kv3
