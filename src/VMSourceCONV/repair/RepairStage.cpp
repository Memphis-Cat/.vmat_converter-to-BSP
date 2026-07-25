#include "repair/RepairStage.h"

namespace vmsourceconv::repair {

const char* RepairStage::Name() noexcept {
    return "repair";
}

bool RepairStage::IsImplemented() noexcept {
    return false;
}

} // namespace vmsourceconv::repair
