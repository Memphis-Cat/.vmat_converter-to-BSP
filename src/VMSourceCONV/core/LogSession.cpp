#include "core/LogSession.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

std::string QuoteArgument(const std::string& value) {
    if (value.find_first_of(" \t\"") == std::string::npos) {
        return value;
    }
    std::string quoted = "\"";
    for (const auto character : value) {
        if (character == '"') {
            quoted += '\\';
        }
        quoted += character;
    }
    quoted += '"';
    return quoted;
}

} // namespace

LogSession::LogSession(const LogOptions& options)
    : options_(options), started_(std::chrono::steady_clock::now()) {
    originalOutput_ = std::cout.rdbuf();
    originalError_ = std::cerr.rdbuf();

    if (!options_.path.empty()) {
        std::error_code error;
        resolvedPath_ = std::filesystem::absolute(options_.path, error);
        if (error) {
            resolvedPath_ = options_.path;
        }
        resolvedPath_ = resolvedPath_.lexically_normal();
        const auto parent = resolvedPath_.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }

        auto mode = std::ios::binary | std::ios::out;
        mode |= options_.mode == LogMode::Append
            ? std::ios::app
            : std::ios::trunc;
        file_.open(resolvedPath_, mode);
        if (!file_) {
            throw std::runtime_error(
                "unable to open log file: " + resolvedPath_.string());
        }
    }

    if (file_ || options_.verbosity == Verbosity::Quiet) {
        outputTee_ = std::make_unique<TeeStreamBuf>(
            options_.verbosity == Verbosity::Quiet ? nullptr : originalOutput_,
            file_ ? file_.rdbuf() : nullptr,
            "INFO",
            options_.timestamps,
            false);
        std::cout.rdbuf(outputTee_.get());
    }
    if (file_) {
        errorTee_ = std::make_unique<TeeStreamBuf>(
            originalError_,
            file_.rdbuf(),
            "ERROR",
            options_.timestamps,
            true);
        std::cerr.rdbuf(errorTee_.get());
    }
}

LogSession::~LogSession() {
    if (!finished_ && file_) {
        Finish(-1);
    }
    RestoreStreams();
    if (file_) {
        file_.flush();
    }
}

void LogSession::WriteHeader(
    const std::string& version,
    const int argc,
    char** argv) {
    if (!file_) {
        return;
    }
    file_ << '[' << Timestamp() << "] [SESSION] VMSourceCONV "
          << version << '\n'
          << '[' << Timestamp() << "] [SESSION] Log mode: "
          << ToString(options_.mode) << '\n'
          << '[' << Timestamp() << "] [SESSION] Verbosity: "
          << ToString(options_.verbosity) << '\n'
          << '[' << Timestamp() << "] [SESSION] Log path: "
          << resolvedPath_.string() << '\n'
          << '[' << Timestamp() << "] [SESSION] Command:";
    for (int index = 0; index < argc; ++index) {
        file_ << ' ' << QuoteArgument(argv[index]);
    }
    file_ << '\n';
    file_.flush();
}

void LogSession::Finish(const int exitCode) {
    if (finished_) {
        return;
    }
    finished_ = true;
    if (file_) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started_);
        file_ << '[' << Timestamp() << "] [SESSION] Exit code: "
              << exitCode << '\n'
              << '[' << Timestamp() << "] [SESSION] Elapsed: "
              << elapsed.count() << " ms\n";
        file_.flush();
    }
}

const std::filesystem::path& LogSession::Path() const noexcept {
    return resolvedPath_;
}

Verbosity LogSession::GetVerbosity() const noexcept {
    return options_.verbosity;
}

void LogSession::RestoreStreams() noexcept {
    if (originalOutput_ != nullptr && std::cout.rdbuf() != originalOutput_) {
        std::cout.rdbuf(originalOutput_);
    }
    if (originalError_ != nullptr && std::cerr.rdbuf() != originalError_) {
        std::cerr.rdbuf(originalError_);
    }
}

} // namespace vmsourceconv::core
