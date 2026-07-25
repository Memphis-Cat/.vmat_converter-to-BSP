#pragma once

#include <string_view>

namespace vmsourceconv::graph {

[[nodiscard]] bool IsStructuralMapResource(std::string_view logicalName);
[[nodiscard]] bool ShouldFollowResource(std::string_view logicalName, bool includeAssets);

} // namespace vmsourceconv::graph
