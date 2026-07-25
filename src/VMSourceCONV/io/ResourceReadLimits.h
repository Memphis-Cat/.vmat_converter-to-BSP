#pragma once

#include <cstdint>

namespace vmsourceconv::io {

struct ResourceReadLimits {
    std::uint64_t maximumInputBytes = 2ULL * 1024ULL * 1024ULL * 1024ULL;
    std::uint64_t maximumVpkEntryBytes = 1024ULL * 1024ULL * 1024ULL;
    std::uint64_t maximumGraphBytes = 8ULL * 1024ULL * 1024ULL * 1024ULL;
};

} // namespace vmsourceconv::io
