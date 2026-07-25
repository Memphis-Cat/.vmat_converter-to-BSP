#pragma once

#include <streambuf>

namespace vmsourceconv::core {

class TeeStreamBuf final : public std::streambuf {
public:
    TeeStreamBuf(std::streambuf* first, std::streambuf* second);

protected:
    int overflow(int character) override;
    std::streamsize xsputn(const char* data, std::streamsize size) override;
    int sync() override;

private:
    std::streambuf* first_;
    std::streambuf* second_;
};

} // namespace vmsourceconv::core
