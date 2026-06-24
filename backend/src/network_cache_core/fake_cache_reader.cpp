#include "kivo/cinema_engine/network_cache_core/fake_cache_reader.hpp"

namespace kivo::cinema_engine {

FakeCacheReadResult read_fake_cache_extent(const CacheExtent& extent,
                                           const RemoteObjectIdentity& identity,
                                           const FakeCacheReadRequest& request) {
  FakeCacheReadResult result;

  if (extent.remote_object_identity_ref != identity.remote_object_identity_id ||
      request.remote_object_identity_ref != identity.remote_object_identity_id) {
    result.reason = "remote object identity mismatch";
    return result;
  }

  if (request.cache_extent_id != extent.cache_extent_id) {
    result.reason = "cache extent mismatch";
    return result;
  }

  if (request.length == 0 || request.offset < extent.start_byte || request.offset > extent.end_byte) {
    result.reason = "requested range outside cache extent";
    return result;
  }

  const std::uint64_t available = extent.end_byte - request.offset + 1;
  result.allowed = true;
  result.readable_bytes = request.length < available ? request.length : available;
  result.reason = "cache read allowed";
  return result;
}

}  // namespace kivo::cinema_engine

