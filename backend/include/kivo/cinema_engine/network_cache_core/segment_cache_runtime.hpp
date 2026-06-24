#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Segment cache runtime bound to a RemoteObjectIdentity.
// Tracks cache fill state and hit rate for network playback.
struct SegmentCacheRuntime {
    ContractMetadata metadata;
    std::string cache_id;
    std::string bound_remote_object_identity_id;
    int64_t total_bytes{0};
    int64_t cached_bytes{0};
    double hit_rate{0.0};
};

}  // namespace kivo::cinema_engine
