#pragma once

#include "core/TeeStreamBuf.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <streambuf>

namespace vmsourceconv::core {

class LogSession final {
public:
    explicit LogSession(const std::filesystem::path& path);
    ~LogSession();

    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;

private:
    std::ofstream file_;
    std::unique_ptr<TeeStreamBuf> outputTee_;
    std::unique_ptr<TeeStreamBuf> errorTee_;
    std::streambuf* originalOutput_ = nullptr;
    std::streambuf* originalError_ = nullptr;
};

} // namespace vmsourceconv::core
