#pragma once

#include <streambuf>
#include <string>

namespace vmsourceconv::core {

class TeeStreamBuf final : public std::streambuf {
public:
    TeeStreamBuf(
        std::streambuf* console,
        std::streambuf* log,
        std::string severity,
        bool timestamps,
        bool flushLogOnLine);

protected:
    int overflow(int character) override;
    std::streamsize xsputn(const char* data, std::streamsize size) override;
    int sync() override;

private:
    bool WriteLogCharacter(char value);
    bool WriteLogPrefix();

    std::streambuf* console_;
    std::streambuf* log_;
    std::string severity_;
    bool timestamps_ = true;
    bool flushLogOnLine_ = false;
    bool logLineStart_ = true;
};

} // namespace vmsourceconv::core
