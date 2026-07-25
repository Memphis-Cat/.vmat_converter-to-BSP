#include "fix/FixStage.h"

namespace vmsourceconv::fix {

const char* FixStage::Name() noexcept {
    return "fix";
}

bool FixStage::IsImplemented() noexcept {
    return false;
}

} // namespace vmsourceconv::fix
