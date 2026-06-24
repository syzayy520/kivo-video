#pragma once

#include <cstdint>
#include <string>

namespace kivo::cinema_engine {

struct CacheExtent {
  std::string cache_extent_id;
  std::string remote_object_identity_ref;
  std::uint64_t start_byte{0};
  std::uint64_t end_byte{0};
};

}  // namespace kivo::cinema_engine

