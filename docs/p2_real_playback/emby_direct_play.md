# P2-037 Emby Direct Play Playback

## Purpose

Model the Emby direct play interaction: requesting direct play URLs for media items, handling transcoding fallback when direct play is not possible, and tracking direct play eligibility.

## Scope

- Direct play request with item_id and media_source_id
- Direct play response with URL and playback info
- Transcoding fallback when direct play is unsupported
- Failure handling (not found, incompatible, server error)

## Family Tree (source_core/)

```
source_core/
├── emby_direct_play_request.hpp      — Request for direct play URL
├── emby_direct_play_response.hpp     — Response with direct play URL or transcode info
├── emby_direct_play_failure.hpp      — Failure kinds for direct play
├── fake_emby_direct_play_service.hpp — Fake service for testing
```

## Contract Types

### EmbyDirectPlayRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique identifier |
| item_id | string | Emby media item identifier |
| media_source_id | string | Media source within the item |
| device_profile_id | string | Client device profile for compatibility check |
| max_streaming_bitrate | int64_t | Maximum acceptable bitrate (0 = unlimited) |

### EmbyDirectPlayResponse

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Correlated request ID |
| direct_play_url | string | Direct play URL (empty if transcoding needed) |
| direct_play_container | string | Container format (mp4, mkv, etc.) |
| transcoding_url | string | Transcoding URL (empty if direct play possible) |
| transcoding_protocol | string | Transcoding protocol (hls, dash, etc.) |
| is_direct_play | bool | true = direct play, false = transcoding |

### EmbyDirectPlayFailure

| Field | Type | Description |
|-------|------|-------------|
| failure_kind | EmbyDirectPlayFailureKind | Kind of failure |
| failure_reason | string | Human-readable reason |
| retry_after_ms | int64_t | Retry delay (0 = no retry) |
| is_recoverable | bool | Whether retry may succeed |

### EmbyDirectPlayFailureKind

| Value | Description |
|-------|-------------|
| ItemNotFound | Media item not found |
| IncompatibleFormat | Client cannot play this format |
| ServerError | Emby server error |

## Tests

10 tests in `backend/tests/source_core/emby_direct_play_test.cpp`:
1. Request construction
2. Direct play response
3. Transcoding fallback response
4. Container format tracking
5. Bitrate limit tracking
6. Item not found failure
7. Incompatible format failure
8. Server error failure
9. Request-response correlation
10. Direct play eligibility check
