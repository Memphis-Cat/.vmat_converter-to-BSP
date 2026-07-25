#pragma once

#include <cstdint>

namespace vmsourceconv::serialization::kv3 {

constexpr std::uint32_t BinaryKv3Magic0 = 0x03564B56U;
constexpr std::uint32_t BinaryKv3Magic1 = 0x4B563301U;
constexpr std::uint32_t BinaryKv3Magic2 = 0x4B563302U;
constexpr std::uint32_t BinaryKv3Magic3 = 0x4B563303U;
constexpr std::uint32_t BinaryKv3Magic4 = 0x4B563304U;
constexpr std::uint32_t BinaryKv3Magic5 = 0x4B563305U;

[[nodiscard]] bool IsBinaryKv3Magic(std::uint32_t magic) noexcept;
[[nodiscard]] int BinaryKv3Version(std::uint32_t magic) noexcept;
[[nodiscard]] const char* BinaryKv3MagicName(std::uint32_t magic) noexcept;

} // namespace vmsourceconv::serialization::kv3
