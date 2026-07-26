#include "core/TeeStreamBuf.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace vmsourceconv::core {
namespace {

std::string Timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

} // namespace

TeeStreamBuf::TeeStreamBuf(
    std::streambuf* console,
    std::streambuf* log,
    std::string severity,
    const bool timestamps,
    const bool flushLogOnLine)
    : console_(console),
      log_(log),
      severity_(std::move(severity)),
      timestamps_(timestamps),
      flushLogOnLine_(flushLogOnLine) {
}

int TeeStreamBuf::overflow(const int character) {
    if (traits_type::eq_int_type(character, traits_type::eof())) {
        return traits_type::not_eof(character);
    }
    const auto value = traits_type::to_char_type(character);
    if (console_ != nullptr
        && traits_type::eq_int_type(console_->sputc(value), traits_type::eof())) {
        return traits_type::eof();
    }
    if (!WriteLogCharacter(value)) {
        return traits_type::eof();
    }
    return character;
}

std::streamsize TeeStreamBuf::xsputn(
    const char* data,
    const std::streamsize size) {
    if (size <= 0) {
        return 0;
    }
    if (console_ != nullptr && console_->sputn(data, size) != size) {
        return 0;
    }
    for (std::streamsize index = 0; index < size; ++index) {
        if (!WriteLogCharacter(data[index])) {
            return index;
        }
    }
    return size;
}

int TeeStreamBuf::sync() {
    const auto consoleResult = console_ == nullptr ? 0 : console_->pubsync();
    const auto logResult = log_ == nullptr ? 0 : log_->pubsync();
    return consoleResult == 0 && logResult == 0 ? 0 : -1;
}

bool TeeStreamBuf::WriteLogCharacter(const char value) {
    if (log_ == nullptr) {
        return true;
    }
    if (logLineStart_ && value != '\n') {
        if (!WriteLogPrefix()) {
            return false;
        }
        logLineStart_ = false;
    }
    if (traits_type::eq_int_type(log_->sputc(value), traits_type::eof())) {
        return false;
    }
    if (value == '\n') {
        logLineStart_ = true;
        if (flushLogOnLine_ && log_->pubsync() != 0) {
            return false;
        }
    }
    return true;
}

bool TeeStreamBuf::WriteLogPrefix() {
    std::string prefix;
    if (timestamps_) {
        prefix += '[' + Timestamp() + "] ";
    }
    if (!severity_.empty()) {
        prefix += '[' + severity_ + "] ";
    }
    return log_->sputn(
        prefix.data(),
        static_cast<std::streamsize>(prefix.size()))
        == static_cast<std::streamsize>(prefix.size());
}

} // namespace vmsourceconv::core
