#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

enum class CacheSegmentStatus {
    Downloading,
    Cached,
    Evicted,
    Invalidated
};

struct CacheSegmentDescriptor {
    ContractMetadata metadata;
    std::string segment_id;
    std::string remote_object_identity_ref;
    uint64_t start_byte{0};
    uint64_t end_byte{0};
    CacheSegmentStatus status{CacheSegmentStatus::Downloading};
    uint64_t size_bytes{0};
    std::string etag;
    int64_t cached_at{0};
};

}  // namespace kivo::cinema_engine
