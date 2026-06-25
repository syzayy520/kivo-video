#include "kivo/cinema_engine/network_cache_core/fake_segment_cache_service.hpp"

namespace kivo::cinema_engine {

SegmentCacheRuntime FakeSegmentCacheService::build_empty_cache() {
    SegmentCacheRuntime r;
    r.metadata.schema_version = "1.1";
    r.cache_id = "cache-empty-001";
    r.bound_remote_object_identity_id = "remote-obj-001";
    r.total_bytes = 5368709120;  // 5 GB total content
    r.cached_bytes = 0;
    r.hit_rate = 0.0;
    r.segment_count = 0;
    r.max_segments = 256;
    r.eviction_policy = "lru";
    r.total_lookups = 0;
    r.cache_hits = 0;
    r.cache_misses = 0;
    r.eviction_count = 0;
    return r;
}

SegmentCacheRuntime FakeSegmentCacheService::build_active_cache() {
    SegmentCacheRuntime r;
    r.metadata.schema_version = "1.1";
    r.cache_id = "cache-active-001";
    r.bound_remote_object_identity_id = "remote-obj-001";
    r.total_bytes = 5368709120;   // 5 GB total content
    r.cached_bytes = 157286400;   // ~150 MB cached
    r.hit_rate = 0.85;
    r.segment_count = 15;
    r.max_segments = 256;
    r.eviction_policy = "lru";
    r.total_lookups = 1000;
    r.cache_hits = 850;
    r.cache_misses = 150;
    r.eviction_count = 12;
    return r;
}

SegmentCacheRuntime FakeSegmentCacheService::build_full_cache() {
    SegmentCacheRuntime r;
    r.metadata.schema_version = "1.1";
    r.cache_id = "cache-full-001";
    r.bound_remote_object_identity_id = "remote-obj-001";
    r.total_bytes = 5368709120;   // 5 GB total content
    r.cached_bytes = 5368709120;  // fully cached
    r.hit_rate = 1.0;
    r.segment_count = 256;
    r.max_segments = 256;
    r.eviction_policy = "lru";
    r.total_lookups = 5000;
    r.cache_hits = 5000;
    r.cache_misses = 0;
    r.eviction_count = 0;
    return r;
}

SegmentCacheRuntime FakeSegmentCacheService::build_degraded_cache() {
    SegmentCacheRuntime r;
    r.metadata.schema_version = "1.1";
    r.cache_id = "cache-degraded-001";
    r.bound_remote_object_identity_id = "remote-obj-001";
    r.total_bytes = 5368709120;
    r.cached_bytes = 31457280;  // ~30 MB cached (poor fill)
    r.hit_rate = 0.35;
    r.segment_count = 256;       // at max segments
    r.max_segments = 256;
    r.eviction_policy = "lru";
    r.total_lookups = 2000;
    r.cache_hits = 350;
    r.cache_misses = 1650;
    r.eviction_count = 450;
    return r;
}

CacheSegmentDescriptor FakeSegmentCacheService::build_cached_segment() {
    CacheSegmentDescriptor s;
    s.metadata.schema_version = "1.1";
    s.segment_id = "seg-cached-001";
    s.remote_object_identity_ref = "remote-obj-001";
    s.start_byte = 0;
    s.end_byte = 1048575;  // 1 MB segment
    s.status = CacheSegmentStatus::Cached;
    s.size_bytes = 1048576;
    s.etag = "\"abc123\"";
    s.cached_at = 1719300000;
    return s;
}

CacheSegmentDescriptor FakeSegmentCacheService::build_downloading_segment() {
    CacheSegmentDescriptor s;
    s.metadata.schema_version = "1.1";
    s.segment_id = "seg-dl-001";
    s.remote_object_identity_ref = "remote-obj-001";
    s.start_byte = 1048576;
    s.end_byte = 2097151;
    s.status = CacheSegmentStatus::Downloading;
    s.size_bytes = 524288;  // half downloaded
    s.etag = "\"abc123\"";
    s.cached_at = 0;
    return s;
}

CacheSegmentDescriptor FakeSegmentCacheService::build_evicted_segment() {
    CacheSegmentDescriptor s;
    s.metadata.schema_version = "1.1";
    s.segment_id = "seg-evicted-001";
    s.remote_object_identity_ref = "remote-obj-001";
    s.start_byte = 4194304;
    s.end_byte = 5242879;
    s.status = CacheSegmentStatus::Evicted;
    s.size_bytes = 0;
    s.etag = "\"abc123\"";
    s.cached_at = 1719290000;
    return s;
}

CacheSegmentDescriptor FakeSegmentCacheService::build_invalidated_segment() {
    CacheSegmentDescriptor s;
    s.metadata.schema_version = "1.1";
    s.segment_id = "seg-invalid-001";
    s.remote_object_identity_ref = "remote-obj-001";
    s.start_byte = 0;
    s.end_byte = 1048575;
    s.status = CacheSegmentStatus::Invalidated;
    s.size_bytes = 1048576;
    s.etag = "\"old-etag\"";  // ETag changed since cached
    s.cached_at = 1719280000;
    return s;
}

CacheLookupResult FakeSegmentCacheService::build_full_hit() {
    CacheLookupResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "lookup-hit-001";
    r.hit = true;
    r.readable_bytes = 1048576;  // full 1 MB served
    r.matched_segment_id = "seg-cached-001";
    r.miss_reason = "";
    return r;
}

CacheLookupResult FakeSegmentCacheService::build_partial_hit() {
    CacheLookupResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "lookup-partial-001";
    r.hit = true;
    r.readable_bytes = 524288;  // only half available
    r.matched_segment_id = "seg-cached-001";
    r.miss_reason = "";
    return r;
}

CacheLookupResult FakeSegmentCacheService::build_miss_no_segment() {
    CacheLookupResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "lookup-miss-001";
    r.hit = false;
    r.readable_bytes = 0;
    r.matched_segment_id = "";
    r.miss_reason = "no matching segment in cache";
    return r;
}

CacheLookupResult FakeSegmentCacheService::build_miss_evicted() {
    CacheLookupResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "lookup-evicted-001";
    r.hit = false;
    r.readable_bytes = 0;
    r.matched_segment_id = "seg-evicted-001";
    r.miss_reason = "segment was evicted";
    return r;
}

CacheLookupResult FakeSegmentCacheService::build_miss_invalidated() {
    CacheLookupResult r;
    r.metadata.schema_version = "1.1";
    r.request_id = "lookup-invalid-001";
    r.hit = false;
    r.readable_bytes = 0;
    r.matched_segment_id = "seg-invalid-001";
    r.miss_reason = "segment was invalidated due to ETag mismatch";
    return r;
}

}  // namespace kivo::cinema_engine
