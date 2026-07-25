#pragma once

#include "io/FileReader.h"
#include "resource/ResourceBlock.h"
#include "resource/ResourceHeader.h"

#include <vector>

namespace vmsourceconv::resource {

struct ResourceDocument {
    io::FileData file;
    ResourceHeader header;
    std::vector<ResourceBlock> blocks;
};

} // namespace vmsourceconv::resource
