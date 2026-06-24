#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "kivo/cinema_engine/network_cache_core/cache_extent.hpp"
#include "kivo/cinema_engine/remote_object_core/remote_object_identity.hpp"

namespace kivo::cinema_engine {

struct FakeCacheReadRequest {
  std::string cache_extent_id;
  std::string remote_object_identity_ref;
  std::uint64_t offset{0};
  std::uint64_t length{0};
};

struct FakeCacheReadResult {
  bool allowed{false};
  std::string reason;
  std::uint64_t readable_bytes{0};
};

FakeCacheReadResult read_fake_cache_extent(const CacheExtent& extent,
                                           const RemoteObjectIdentity& identity,
                                           const FakeCacheReadRequest& request);

}  // namespace kivo::cinema_engine

