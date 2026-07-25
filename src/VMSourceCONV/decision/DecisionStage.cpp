#include "decision/DecisionStage.h"

namespace vmsourceconv::decision {

const char* DecisionStage::Name() noexcept {
    return "decision";
}

bool DecisionStage::IsImplemented() noexcept {
    return false;
}

} // namespace vmsourceconv::decision
