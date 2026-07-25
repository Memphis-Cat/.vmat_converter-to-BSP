#include "compiling/CompilationStage.h"

namespace vmsourceconv::compiling {

const char* CompilationStage::Name() noexcept {
    return "compiling";
}

bool CompilationStage::IsImplemented() noexcept {
    return false;
}

} // namespace vmsourceconv::compiling
