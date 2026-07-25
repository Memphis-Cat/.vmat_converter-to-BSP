#include "inspect/data/DataBlockInspector.h"

#include "inspect/data/DataFormatDetector.h"

namespace vmsourceconv::inspect::data {

DataInspectionResult DataBlockInspector::Inspect(
    const resource::ResourceDocument& document) const {
    for (const auto& block : document.blocks) {
        if (block.type.ToString() == "DATA") {
            return DataFormatDetector{}.Detect(
                document.file.bytes,
                block.offset,
                block.size);
        }
    }

    DataInspectionResult result;
    result.message = "resource has no DATA block";
    return result;
}

} // namespace vmsourceconv::inspect::data
