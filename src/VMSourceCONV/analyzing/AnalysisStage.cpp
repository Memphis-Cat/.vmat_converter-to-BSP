#include "analyzing/AnalysisStage.h"

namespace vmsourceconv::analyzing {

const char* AnalysisStage::Name() noexcept {
    return "analyzing";
}

bool AnalysisStage::IsImplemented() noexcept {
    return false;
}

} // namespace vmsourceconv::analyzing
