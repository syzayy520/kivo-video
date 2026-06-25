# P2-039 Subtitle Runtime

## Purpose

Model the subtitle runtime: loading external subtitle files (SRT/ASS), tracking subtitle state, and providing subtitle data for overlay rendering.

## Scope

- Subtitle file loading and parsing (SRT, ASS)
- Subtitle state tracking (loaded, active, hidden)
- Subtitle timing and synchronization
- Subtitle rendering data for overlay

## Family Tree (subtitle_core/)

```
subtitle_core/
├── subtitle_track_info.hpp        — Metadata for a subtitle track
├── subtitle_entry.hpp             — Individual subtitle entry with timing
├── subtitle_load_result.hpp       — Result of loading a subtitle file
├── fake_subtitle_runtime_service.hpp — Fake service for testing
```

## Contract Types

### SubtitleTrackInfo

| Field | Type | Description |
|-------|------|-------------|
| track_id | string | Unique track identifier |
| file_path | string | Path to subtitle file |
| format | string | Subtitle format (srt, ass, ssa) |
| language | string | Language code (en, zh, ja, etc.) |
| is_default | bool | Whether this is the default track |
| entry_count | int64_t | Number of subtitle entries |

### SubtitleEntry

| Field | Type | Description |
|-------|------|-------------|
| entry_id | int64_t | Entry sequence number |
| start_time_ms | int64_t | Start time in milliseconds |
| end_time_ms | int64_t | End time in milliseconds |
| text | string | Subtitle text content |
| style | string | ASS style name (empty for SRT) |

### SubtitleLoadResult

| Field | Type | Description |
|-------|------|-------------|
| success | bool | Whether loading succeeded |
| track_info | SubtitleTrackInfo | Track metadata |
| entries | vector<SubtitleEntry> | Loaded entries |
| error_message | string | Error if loading failed |

### SubtitleRuntimeState

| Value | Description |
|-------|-------------|
| Unloaded | No subtitle loaded |
| Loaded | Subtitle loaded, not yet active |
| Active | Subtitle actively displayed |
| Hidden | Subtitle loaded but hidden by user |

## Tests

10 tests in `backend/tests/subtitle_core/subtitle_runtime_test.cpp`:
1. SRT track info construction
2. ASS track info construction
3. Subtitle entry construction
4. SRT load result success
5. ASS load result success
6. Load result with error
7. Runtime state transitions
8. Entry timing validity
9. Language code tracking
10. Default track flag
