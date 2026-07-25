#include "resource/FourCC.h"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace vmsourceconv::resource {

FourCC::FourCC(const std::uint32_t value) : value_(value) {}

std::uint32_t FourCC::Value() const noexcept {
    return value_;
}

std::string FourCC::ToString() const {
    std::string text(4, ' ');
    bool printable = true;

    for (std::size_t index = 0; index < 4; ++index) {
        const auto byte = static_cast<unsigned char>((value_ >> (index * 8U)) & 0xFFU);
        printable = printable && std::isprint(byte) != 0;
        text[index] = static_cast<char>(byte);
    }

    if (printable) {
        return text;
    }

    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value_;
    return output.str();
}

bool FourCC::Is(const char (&text)[5]) const noexcept {
    std::uint32_t expected = 0;
    for (std::size_t index = 0; index < 4; ++index) {
        expected |= static_cast<std::uint32_t>(static_cast<unsigned char>(text[index])) << (index * 8U);
    }
    return value_ == expected;
}

} // namespace vmsourceconv::resource
