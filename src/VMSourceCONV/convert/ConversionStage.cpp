#include "convert/ConversionStage.h"

namespace vmsourceconv::convert {

const char* ConversionStage::Name() noexcept {
    return "convert";
}

bool ConversionStage::IsImplemented() noexcept {
    return false;
}

} // namespace vmsourceconv::convert
