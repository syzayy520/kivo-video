#pragma once

#include <cstdint>

namespace kivo::cinema::foundation::schema {

struct SchemaVersion final {
  std::uint16_t major{1};
  std::uint16_t minor{0};
  std::uint16_t patch{0};
};

inline constexpr bool is_valid(const SchemaVersion& version) noexcept {
  return version.major > 0;
}

}  // namespace kivo::cinema::foundation::schema
