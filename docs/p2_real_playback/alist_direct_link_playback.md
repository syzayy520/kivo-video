# P2-035 AList Direct Link Playback

## Purpose

Model the AList direct link playback interaction: generating time-limited download links, refreshing them before expiry (TTL management), and maintaining stable resource identity across link refreshes.

## Scope

- Direct link request and response
- TTL tracking and refresh strategy
- Identity stability (same resource identity across link rotations)
- Failure handling (link expired, server unreachable)

## Family Tree (source_core/)

```
source_core/
├── alist_direct_link_request.hpp      — Request for a direct download link
├── alist_direct_link_response.hpp     — Response with link URL and TTL
├── alist_direct_link_identity.hpp     — Stable identity across link refreshes
├── fake_alist_direct_link_service.hpp — Fake service for testing
```

## Contract Types

### AListDirectLinkRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique identifier |
| file_path | string | Path in AList storage |
| auth_token | string | Authentication token |
| expires_in_seconds | int64_t | Requested TTL (0 = server default) |

### AListDirectLinkResponse

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Correlated request ID |
| direct_link_url | string | Time-limited download URL |
| ttl_seconds | int64_t | Actual TTL granted by server |
| content_type | string | MIME type of the resource |
| content_length | int64_t | Size in bytes |

### AListDirectLinkIdentity

| Field | Type | Description |
|-------|------|-------------|
| identity_id | string | Stable identity (unchanged across refreshes) |
| file_path | string | Path in AList storage |
| content_hash | string | Content hash for deduplication |
| first_link_generated_at | int64_t | Timestamp of first link generation |
| link_refresh_count | int64_t | Number of link refreshes so far |
| last_refresh_at | int64_t | Timestamp of last refresh |

## Tests

10 tests in `backend/tests/source_core/alist_direct_link_test.cpp`
