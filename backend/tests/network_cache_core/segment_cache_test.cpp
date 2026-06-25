#include <cassert>
#include <iostream>
#include <string>

#include "kivo/cinema_engine/network_cache_core/fake_segment_cache_service.hpp"
#include "kivo/cinema_engine/network_cache_core/cache_lookup_request.hpp"

using namespace kivo::cinema_engine;

// P2-031: Segment Cache Runtime tests

void test_empty_cache_runtime() {
    auto cache = FakeSegmentCacheService::build_empty_cache();
    assert(cache.metadata.schema_version == "1.1");
    assert(cache.cache_id == "cache-empty-001");
    assert(cache.bound_remote_object_identity_id == "remote-obj-001");
    assert(cache.total_bytes == 5368709120);
    assert(cache.cached_bytes == 0);
    assert(cache.hit_rate == 0.0);
    assert(cache.segment_count == 0);
    assert(cache.max_segments == 256);
    assert(cache.eviction_policy == "lru");
    assert(cache.total_lookups == 0);
    assert(cache.cache_hits == 0);
    assert(cache.cache_misses == 0);
    assert(cache.eviction_count == 0);
    std::cout << "PASS: test_empty_cache_runtime" << std::endl;
}

void test_active_cache_runtime() {
    auto cache = FakeSegmentCacheService::build_active_cache();
    assert(cache.cache_id == "cache-active-001");
    assert(cache.cached_bytes == 157286400);
    assert(cache.hit_rate == 0.85);
    assert(cache.segment_count == 15);
    assert(cache.total_lookups == 1000);
    assert(cache.cache_hits == 850);
    assert(cache.cache_misses == 150);
    assert(cache.eviction_count == 12);
    std::cout << "PASS: test_active_cache_runtime" << std::endl;
}

void test_full_cache_runtime() {
    auto cache = FakeSegmentCacheService::build_full_cache();
    assert(cache.cached_bytes == cache.total_bytes);
    assert(cache.hit_rate == 1.0);
    assert(cache.segment_count == 256);
    assert(cache.max_segments == 256);
    assert(cache.cache_hits == 5000);
    assert(cache.cache_misses == 0);
    std::cout << "PASS: test_full_cache_runtime" << std::endl;
}

void test_degraded_cache_runtime() {
    auto cache = FakeSegmentCacheService::build_degraded_cache();
    assert(cache.hit_rate == 0.35);
    assert(cache.segment_count == 256);  // at max
    assert(cache.eviction_count == 450);
    // High eviction count with low hit rate = degraded
    assert(cache.eviction_count > cache.cache_hits);
    std::cout << "PASS: test_degraded_cache_runtime" << std::endl;
}

void test_cached_segment() {
    auto seg = FakeSegmentCacheService::build_cached_segment();
    assert(seg.metadata.schema_version == "1.1");
    assert(seg.segment_id == "seg-cached-001");
    assert(seg.remote_object_identity_ref == "remote-obj-001");
    assert(seg.start_byte == 0);
    assert(seg.end_byte == 1048575);
    assert(seg.status == CacheSegmentStatus::Cached);
    assert(seg.size_bytes == 1048576);
    assert(seg.etag == "\"abc123\"");
    assert(seg.cached_at > 0);
    std::cout << "PASS: test_cached_segment" << std::endl;
}

void test_downloading_segment() {
    auto seg = FakeSegmentCacheService::build_downloading_segment();
    assert(seg.status == CacheSegmentStatus::Downloading);
    assert(seg.size_bytes == 524288);  // half downloaded
    assert(seg.cached_at == 0);        // not yet complete
    std::cout << "PASS: test_downloading_segment" << std::endl;
}

void test_evicted_segment() {
    auto seg = FakeSegmentCacheService::build_evicted_segment();
    assert(seg.status == CacheSegmentStatus::Evicted);
    assert(seg.size_bytes == 0);  // no data retained
    std::cout << "PASS: test_evicted_segment" << std::endl;
}

void test_invalidated_segment() {
    auto seg = FakeSegmentCacheService::build_invalidated_segment();
    assert(seg.status == CacheSegmentStatus::Invalidated);
    assert(seg.etag == "\"old-etag\"");
    std::cout << "PASS: test_invalidated_segment" << std::endl;
}

void test_full_cache_hit() {
    auto result = FakeSegmentCacheService::build_full_hit();
    assert(result.metadata.schema_version == "1.1");
    assert(result.request_id == "lookup-hit-001");
    assert(result.hit == true);
    assert(result.readable_bytes == 1048576);
    assert(result.matched_segment_id == "seg-cached-001");
    assert(result.miss_reason.empty());
    std::cout << "PASS: test_full_cache_hit" << std::endl;
}

void test_partial_cache_hit() {
    auto result = FakeSegmentCacheService::build_partial_hit();
    assert(result.hit == true);
    assert(result.readable_bytes == 524288);  // partial
    assert(!result.matched_segment_id.empty());
    std::cout << "PASS: test_partial_cache_hit" << std::endl;
}

void test_miss_no_segment() {
    auto result = FakeSegmentCacheService::build_miss_no_segment();
    assert(result.hit == false);
    assert(result.readable_bytes == 0);
    assert(result.matched_segment_id.empty());
    assert(result.miss_reason == "no matching segment in cache");
    std::cout << "PASS: test_miss_no_segment" << std::endl;
}

void test_miss_evicted() {
    auto result = FakeSegmentCacheService::build_miss_evicted();
    assert(result.hit == false);
    assert(!result.matched_segment_id.empty());
    assert(result.miss_reason == "segment was evicted");
    std::cout << "PASS: test_miss_evicted" << std::endl;
}

void test_miss_invalidated() {
    auto result = FakeSegmentCacheService::build_miss_invalidated();
    assert(result.hit == false);
    assert(!result.matched_segment_id.empty());
    assert(result.miss_reason.find("invalidated") != std::string::npos);
    std::cout << "PASS: test_miss_invalidated" << std::endl;
}

void test_lookup_request_construction() {
    CacheLookupRequest req;
    req.metadata.schema_version = "1.1";
    req.request_id = "req-001";
    req.cache_id = "cache-active-001";
    req.remote_object_identity_ref = "remote-obj-001";
    req.byte_offset = 0;
    req.byte_length = 1048576;
    assert(req.request_id == "req-001");
    assert(req.cache_id == "cache-active-001");
    assert(req.byte_offset == 0);
    assert(req.byte_length == 1048576);
    std::cout << "PASS: test_lookup_request_construction" << std::endl;
}

void test_cache_bound_to_remote_object_identity() {
    auto cache = FakeSegmentCacheService::build_active_cache();
    assert(!cache.bound_remote_object_identity_id.empty());
    assert(cache.bound_remote_object_identity_id == "remote-obj-001");
    std::cout << "PASS: test_cache_bound_to_remote_object_identity" << std::endl;
}

int main() {
    // Runtime tests
    test_empty_cache_runtime();
    test_active_cache_runtime();
    test_full_cache_runtime();
    test_degraded_cache_runtime();

    // Segment descriptor tests
    test_cached_segment();
    test_downloading_segment();
    test_evicted_segment();
    test_invalidated_segment();

    // Lookup result tests
    test_full_cache_hit();
    test_partial_cache_hit();
    test_miss_no_segment();
    test_miss_evicted();
    test_miss_invalidated();

    // Request and binding tests
    test_lookup_request_construction();
    test_cache_bound_to_remote_object_identity();

    std::cout << "All P2-031 Segment Cache Runtime tests passed." << std::endl;
    return 0;
}
