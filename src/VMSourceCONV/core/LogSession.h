#pragma once

#include "core/LogOptions.h"
#include "core/TeeStreamBuf.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <streambuf>
#include <string>

namespace vmsourceconv::core {

class LogSession final {
public:
    explicit LogSession(const LogOptions& options);
    ~LogSession();

    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;

    void WriteHeader(
        const std::string& version,
        int argc,
        char** argv);
    void Finish(int exitCode);

    [[nodiscard]] const std::filesystem::path& Path() const noexcept;
    [[nodiscard]] Verbosity GetVerbosity() const noexcept;

private:
    void RestoreStreams() noexcept;

    LogOptions options_;
    std::filesystem::path resolvedPath_;
    std::ofstream file_;
    std::unique_ptr<TeeStreamBuf> outputTee_;
    std::unique_ptr<TeeStreamBuf> errorTee_;
    std::streambuf* originalOutput_ = nullptr;
    std::streambuf* originalError_ = nullptr;
    std::chrono::steady_clock::time_point started_;
    bool finished_ = false;
};

} // namespace vmsourceconv::core
