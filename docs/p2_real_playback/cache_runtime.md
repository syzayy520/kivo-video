# P2-031 Segment Cache Runtime

## Purpose

Define the segment cache runtime contract for network playback. The segment cache is bound to a RemoteObjectIdentity and tracks cached byte ranges, lookup hit/miss rates, and eviction policy. This enables efficient range-based cache management for WebDAV, AList, and Emby direct play scenarios.

## Contract Design

### CacheSegmentDescriptor

Describes a single cached byte-range segment.

| Field | Type | Description |
|---|---|---|
| segment_id | string | Unique segment identifier |
| remote_object_identity_ref | string | Bound RemoteObjectIdentity ID |
| start_byte | uint64_t | Inclusive start offset |
| end_byte | uint64_t | Inclusive end offset |
| status | CacheSegmentStatus | Downloading / Cached / Evicted / Invalidated |
| size_bytes | uint64_t | Actual bytes stored |
| etag | string | Source ETag at time of caching |
| cached_at | int64_t | Timestamp when segment was cached |

### CacheSegmentStatus

- Downloading: segment is being fetched from remote
- Cached: segment is fully cached and readable
- Evicted: segment was removed by eviction policy
- Invalidated: segment was invalidated (e.g., ETag mismatch)

### SegmentCacheRuntime

Extended from placeholder. Now includes segment count, eviction policy, lookup statistics.

| Field | Type | Default |
|---|---|---|
| metadata | ContractMetadata | - |
| cache_id | string | - |
| bound_remote_object_identity_id | string | - |
| total_bytes | int64_t | 0 |
| cached_bytes | int64_t | 0 |
| hit_rate | double | 0.0 |
| segment_count | int | 0 |
| max_segments | int | 256 |
| eviction_policy | string | "lru" |
| total_lookups | int64_t | 0 |
| cache_hits | int64_t | 0 |
| cache_misses | int64_t | 0 |
| eviction_count | int64_t | 0 |

### CacheLookupRequest

| Field | Type | Description |
|---|---|---|
| request_id | string | Unique request ID |
| cache_id | string | Target cache instance |
| remote_object_identity_ref | string | Target remote object |
| byte_offset | uint64_t | Requested offset |
| byte_length | uint64_t | Requested length |

### CacheLookupResult

| Field | Type | Description |
|---|---|---|
| request_id | string | Echoed request ID |
| hit | bool | Whether cache had the data |
| readable_bytes | uint64_t | Bytes available from cache |
| matched_segment_id | string | Segment that served the request (empty if miss) |
| miss_reason | string | Reason for cache miss |

## Fake Service Scenarios

1. **Full Cache Hit** - segment fully covers request range
2. **Partial Cache Hit** - segment covers part of request, returns partial bytes
3. **Cache Miss** - no segment covers the requested range
4. **Evicted Segment** - segment existed but was evicted
5. **Invalidated Segment** - segment existed but ETag changed

## Files

- `backend/include/kivo/cinema_engine/network_cache_core/cache_segment_descriptor.hpp`
- `backend/include/kivo/cinema_engine/network_cache_core/cache_lookup_request.hpp`
- `backend/include/kivo/cinema_engine/network_cache_core/cache_lookup_result.hpp`
- `backend/include/kivo/cinema_engine/network_cache_core/segment_cache_runtime.hpp` (modified)
- `backend/include/kivo/cinema_engine/network_cache_core/fake_segment_cache_service.hpp`
- `backend/src/network_cache_core/fake_segment_cache_service.cpp`
- `backend/tests/network_cache_core/segment_cache_test.cpp`
