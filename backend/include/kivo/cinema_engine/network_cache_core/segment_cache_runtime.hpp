#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Segment cache runtime bound to a RemoteObjectIdentity.
// Tracks cache fill state, segment count, eviction policy, and hit rate
// for network playback range caching.
struct SegmentCacheRuntime {
    ContractMetadata metadata;
    std::string cache_id;
    std::string bound_remote_object_identity_id;
    int64_t total_bytes{0};
    int64_t cached_bytes{0};
    double hit_rate{0.0};
    int segment_count{0};
    int max_segments{256};
    std::string eviction_policy{"lru"};
    int64_t total_lookups{0};
    int64_t cache_hits{0};
    int64_t cache_misses{0};
    int64_t eviction_count{0};
};

}  // namespace kivo::cinema_engine
