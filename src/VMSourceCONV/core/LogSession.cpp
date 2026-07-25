#include "core/LogSession.h"

#include <iostream>
#include <stdexcept>

namespace vmsourceconv::core {

LogSession::LogSession(const std::filesystem::path& path) {
    if (path.empty()) {
        return;
    }

    file_.open(path, std::ios::binary | std::ios::trunc);
    if (!file_) {
        throw std::runtime_error(
            "unable to open log file: " + path.string());
    }

    originalOutput_ = std::cout.rdbuf();
    originalError_ = std::cerr.rdbuf();

    outputTee_ = std::make_unique<TeeStreamBuf>(
        originalOutput_,
        file_.rdbuf());
    errorTee_ = std::make_unique<TeeStreamBuf>(
        originalError_,
        file_.rdbuf());

    std::cout.rdbuf(outputTee_.get());
    std::cerr.rdbuf(errorTee_.get());
}

LogSession::~LogSession() {
    if (originalOutput_ != nullptr) {
        std::cout.rdbuf(originalOutput_);
    }

    if (originalError_ != nullptr) {
        std::cerr.rdbuf(originalError_);
    }

    if (file_) {
        file_.flush();
    }
}

} // namespace vmsourceconv::core
