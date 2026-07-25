#include "core/TeeStreamBuf.h"

#include <algorithm>

namespace vmsourceconv::core {

TeeStreamBuf::TeeStreamBuf(std::streambuf* first, std::streambuf* second)
    : first_(first), second_(second) {
}

int TeeStreamBuf::overflow(const int character) {
    if (traits_type::eq_int_type(character, traits_type::eof())) {
        return traits_type::not_eof(character);
    }

    const auto value = traits_type::to_char_type(character);
    const auto firstResult = first_->sputc(value);
    const auto secondResult = second_->sputc(value);

    if (traits_type::eq_int_type(firstResult, traits_type::eof())
        || traits_type::eq_int_type(secondResult, traits_type::eof())) {
        return traits_type::eof();
    }

    return character;
}

std::streamsize TeeStreamBuf::xsputn(
    const char* data,
    const std::streamsize size) {
    return std::min(
        first_->sputn(data, size),
        second_->sputn(data, size));
}

int TeeStreamBuf::sync() {
    return first_->pubsync() == 0 && second_->pubsync() == 0 ? 0 : -1;
}

} // namespace vmsourceconv::core
