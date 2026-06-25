#pragma once

#include <string>

#include "kivo/cinema_engine/network_cache_core/segment_cache_runtime.hpp"
#include "kivo/cinema_engine/network_cache_core/cache_segment_descriptor.hpp"
#include "kivo/cinema_engine/network_cache_core/cache_lookup_request.hpp"
#include "kivo/cinema_engine/network_cache_core/cache_lookup_result.hpp"

namespace kivo::cinema_engine {

// Fake segment cache service for P2-031 testing.
// Provides deterministic builder functions for cache runtime, segments, and lookups.
class FakeSegmentCacheService {
public:
    // SegmentCacheRuntime builders
    static SegmentCacheRuntime build_empty_cache();
    static SegmentCacheRuntime build_active_cache();
    static SegmentCacheRuntime build_full_cache();
    static SegmentCacheRuntime build_degraded_cache();

    // CacheSegmentDescriptor builders
    static CacheSegmentDescriptor build_cached_segment();
    static CacheSegmentDescriptor build_downloading_segment();
    static CacheSegmentDescriptor build_evicted_segment();
    static CacheSegmentDescriptor build_invalidated_segment();

    // CacheLookupResult builders
    static CacheLookupResult build_full_hit();
    static CacheLookupResult build_partial_hit();
    static CacheLookupResult build_miss_no_segment();
    static CacheLookupResult build_miss_evicted();
    static CacheLookupResult build_miss_invalidated();
};

}  // namespace kivo::cinema_engine
