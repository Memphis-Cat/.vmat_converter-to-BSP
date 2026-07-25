#include "resource/ResourceBlock.h"

#include "resource/RelativeOffset.h"

namespace vmsourceconv::resource {

std::size_t ResourceBlock::EndOffset() const {
    return CheckedAdd(offset, static_cast<std::size_t>(size));
}

} // namespace vmsourceconv::resource
