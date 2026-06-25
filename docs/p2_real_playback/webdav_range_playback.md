# P2-033 WebDAV Range Playback

## Purpose

Model the byte-range playback interaction contract for WebDAV sources. When playing content from a WebDAV server, the player requests specific byte ranges via HTTP Range headers and must handle full-content, partial-content, not-satisfiable, and fallback scenarios gracefully.

## Scope

- Range request construction (URL, offset, length, headers)
- Range response interpretation (status code, content-range parsing, actual byte count)
- Fallback strategy when the server doesn't support range playback
- Server-side range constraints (chunk limits, single-byte-only quirks)
- Integration with existing P2-032 WebDavRangeBehavior profile

## Family Tree (source_core/)

```
source_core/
├── webdav_range_request.hpp      — Request to read a byte range from a WebDAV URL
├── webdav_range_response.hpp     — Response from a range request
├── webdav_range_fallback.hpp     — Fallback strategy when range unsupported
├── fake_webdav_range_playback_service.hpp — Fake service for testing
```

## Contract Types

### WebDavRangeRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique identifier |
| url | string | Full WebDAV URL |
| byte_offset | int64_t | Start byte position |
| byte_length | int64_t | Number of bytes requested |
| etag | string | Optional: cached ETag for conditional request |
| if_range | string | Optional: If-Range header value |

### WebDavRangeResponse

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Correlated request ID |
| status_code | int | HTTP status (200, 206, 416, etc.) |
| content_range_total | int64_t | Total resource size from Content-Range header |
| content_range_start | int64_t | Start byte from Content-Range header |
| content_range_end | int64_t | End byte from Content-Range header |
| actual_byte_count | int64_t | Actual bytes received in body |
| is_full_content | bool | True if 200 (full body, not ranged) |

### WebDavRangeFallback

| Field | Type | Description |
|-------|------|-------------|
| fallback_id | string | Unique identifier |
| original_request_url | string | The URL that failed range support |
| fallback_strategy | WebDavFallbackStrategy | Enum: full_download, seek_to_zero, abort |
| estimated_download_size | int64_t | Size if full download is needed |
| user_visible_hint | string | Message to display |

### WebDavFallbackStrategy (enum)

- full_download: Download entire resource
- seek_to_zero: Reset to byte 0 and read sequentially
- abort: Abort playback, report error

## Scenarios Tested

1. Successful range read (206 Partial Content)
2. Full content returned (200 OK, range ignored)
3. Range not satisfiable (416)
4. Server does not support range (profile.supports_range_read == false)
5. Chunk size limit exceeded
6. Fallback to full download
7. Fallback abort
8. Conditional request with ETag match (304)
9. Request construction validation

## Tests

14 tests in `backend/tests/source_core/webdav_range_playback_test.cpp`
