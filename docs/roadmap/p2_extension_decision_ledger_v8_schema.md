# P2 Extension Decision Ledger V8 Schema

Updated: 2026-06-26

This document defines the `p2-extension-decision-ledger-v8` schema for extension decisions that stay outside `MediaSourceKind` vocabulary.

## Schema Name

`p2-extension-decision-ledger-v8`

## Purpose

Extension decisions cover Cast, PiP, Cinema, Trakt, playlist, queue, resume, playback history decisions that are not part of the core MediaSourceKind decision vocabulary.

## Top-Level Fields

1. `schema_version` (string, required): Always "p2-extension-decision-ledger-v8".
2. `task_id` (string, required): Task identifier (e.g., "CAST-001", "PIP-001", "CIN-001", "TRK-001").
3. `task_name` (string, required): Human-readable task name.
4. `family` (string, required): Family name matching task ID (e.g., "presentation_extension").
5. `status` (string, required): Evidence status from V8 vocabulary.
6. `runtime_mode` (string, required): Runtime mode from V8 vocabulary.
7. `runtime_environment_kind` (string, required): Environment kind from V8 vocabulary.
8. `environment_required` (boolean, required): Whether real environment is required.
9. `environment_available` (boolean, required): Whether real environment is available.
10. `blocked_reason` (string, nullable): Reason if blocked.
11. `runtime_verified_timestamp` (string, required): ISO 8601 timestamp.
12. `evidence_file` (string, required): Path to this evidence file.
13. `provider_matrix_impact` (object, required): Impact on provider matrix.

## Extension Decision Types

### Cast Decision Types
1. `cast_device_discovery`: Discovery of cast devices on network.
2. `cast_session_handoff`: Handoff from local to cast playback.
3. `cast_protocol_selection`: Selection of cast protocol (AirPlay, Google Cast, etc.).
4. `cast_quality_negotiation`: Quality negotiation with cast device.
5. `cast_error_recovery`: Recovery from cast connection errors.

### PiP Decision Types
6. `pip_surface_detach`: Detach video surface for PiP mode.
7. `pip_surface_attach`: Reattach video surface from PiP mode.
8. `pip_size_change`: Change PiP window size.
9. `pip_position_change`: Change PiP window position.
10. `pip_restore`: Restore from PiP to main window.

### Cinema Decision Types
11. `cinema_mode_enter`: Enter cinema/fullscreen mode.
12. `cinema_mode_exit`: Exit cinema/fullscreen mode.
13. `cinema_chrome_visibility`: Show/hide playback chrome.
14. `cinema_cursor_visibility`: Show/hide cursor in cinema mode.
15. `cinema_osd_safe_area`: OSD safe area boundaries.

### Trakt Decision Types
16. `trakt_scrobble_start`: Start scrobbling to Trakt.
17. `trakt_scrobble_pause`: Pause scrobbling.
18. `trakt_scrobble_stop`: Stop scrobbling.
19. `trakt_watched_mark`: Mark item as watched on Trakt.
20. `trakt_collection_sync`: Sync collection with Trakt.

### Playlist Decision Types
21. `playlist_item_next`: Move to next item in playlist.
22. `playlist_item_previous`: Move to previous item in playlist.
23. `playlist_shuffle_enable`: Enable shuffle mode.
24. `playlist_repeat_mode`: Set repeat mode (none, one, all).
25. `playlist_item_remove`: Remove item from playlist.

### Queue Decision Types
26. `queue_item_add`: Add item to playback queue.
27. `queue_item_remove`: Remove item from playback queue.
28. `queue_item_reorder`: Reorder items in queue.
29. `queue_clear`: Clear entire queue.
30. `queue_persist`: Persist queue to storage.

### Resume Decision Types
31. `resume_point_save`: Save resume point.
32. `resume_point_load`: Load resume point.
33. `resume_point_delete`: Delete resume point.
34. `resume_auto_resume`: Auto-resume on playback start.
35. `resume_cross_device_sync`: Sync resume points across devices.

### Playback History Decision Types
36. `history_record`: Record playback in history.
37. `history_clear`: Clear playback history.
38. `history_export`: Export playback history.
39. `history_import`: Import playback history.
40. `history_prune`: Prune old history entries.

## Extension Stages

1. `cast`: Cast-related decisions.
2. `pip`: Picture-in-picture decisions.
3. `cinema`: Cinema/fullscreen decisions.
4. `trakt`: Trakt integration decisions.
5. `playlist`: Playlist management decisions.
6. `queue`: Queue management decisions.
7. `resume`: Resume point decisions.
8. `history`: Playback history decisions.

## Entry Fields

Each extension decision entry contains:

1. `decision_id` (string, required): Unique decision identifier.
2. `decision_type` (string, required): One of the extension decision types above.
3. `stage` (string, required): One of the extension stages above.
4. `timestamp` (string, required): ISO 8601 timestamp of decision.
5. `input` (object, required): Decision input parameters.
6. `output` (object, required): Decision output/result.
7. `generation` (integer, required): Monotonic generation counter.
8. `redaction_state` (string, required): Redaction state.
9. `provider_impact` (object, optional): Impact on provider matrix.
10. `runtime_status` (string, required): Runtime status from V8 vocabulary.
11. `reason` (string, required): Human-readable reason for decision.
12. `error_info` (object, optional): Error information if decision failed.

## Input/Output Structures

### Cast Device Discovery Input
```json
{
  "discovery_protocol": "airplay|google_cast|dlna",
  "network_interface": "string",
  "timeout_ms": "integer"
}
```

### Cast Device Discovery Output
```json
{
  "devices_found": "integer",
  "device_list": "array of device objects",
  "discovery_duration_ms": "integer"
}
```

### PiP Surface Detach Input
```json
{
  "surface_id": "string",
  "pip_position": {"x": "integer", "y": "integer"},
  "pip_size": {"width": "integer", "height": "integer"}
}
```

### PiP Surface Detach Output
```json
{
  "detach_success": "boolean",
  "main_surface_id": "string",
  "pip_surface_id": "string"
}
```

### Cinema Mode Enter Input
```json
{
  "mode": "fullscreen|cinema",
  "display_id": "string",
  "chrome_visibility": "boolean",
  "cursor_visibility": "boolean"
}
```

### Cinema Mode Enter Output
```json
{
  "enter_success": "boolean",
  "previous_mode": "string",
  "safe_area": {"top": "integer", "right": "integer", "bottom": "integer", "left": "integer"}
}
```

### Trakt Scrobble Start Input
```json
{
  "media_id": "string",
  "media_type": "movie|episode",
  "progress": "float",
  "app_version": "string"
}
```

### Trakt Scrobble Start Output
```json
{
  "scrobble_id": "string",
  "started_at": "string",
  "response_code": "integer"
}
```

## Validation Rules

1. All required top-level fields must be present.
2. `decision_type` must be from the extension decision vocabulary.
3. `stage` must be from the extension stage vocabulary.
4. `runtime_status` must be from V8 status vocabulary.
5. `generation` must be monotonic increasing within task.
6. `redaction_state` must be one of: "none", "partial", "full".
7. Extension decisions cannot use `MediaSourceKind` decision types.
8. Cast decisions require `cast` stage, PiP decisions require `pip` stage, etc.

## Negative Test Cases

1. Cast decision with `source_selection` type → FAIL (wrong decision type).
2. PiP decision with `cast` stage → FAIL (wrong stage).
3. Resume decision with `RUNTIME_PASS` but `environment_available: false` → FAIL.
4. Playlist decision with non-monotonic generation → FAIL.
5. Trakt decision with `redaction_state: "none"` when credentials present → FAIL.
6. Cinema decision with `MediaSourceKind` type → FAIL (extension boundary violation).

## Relationships to Other Schemas

- **p2-evidence-v8**: Extension decisions use same status vocabulary.
- **p2-inspector-v8**: Inspector snapshot includes extension info objects.
- **p2-decision-ledger-v8**: Core decisions use MediaSourceKind vocabulary; extension decisions use extension vocabulary.
- **provider-runtime-matrix-v8**: Extension decisions may impact provider matrix (e.g., cast device availability).

## Provider Matrix Impact

Extension decisions can impact provider matrix entries:
- `airplay_cast`: Cast device availability affects provider runtime status.
- `trakt_api`: Trakt API availability affects provider runtime status.
- Other providers: No direct impact from extension decisions.

## Anti-Fake Rules

1. Cast decisions cannot claim `RUNTIME_PASS` without real cast device.
2. PiP decisions cannot claim `RUNTIME_PASS` without real surface detach/attach.
3. Cinema decisions cannot claim `RUNTIME_PASS` without real display mode change.
4. Trakt decisions cannot claim `RUNTIME_PASS` without real Trakt API.
5. Playlist/queue/resume/history decisions can claim `RUNTIME_PASS` with local state.
6. No extension decision can claim P2 Foundation completion.

## Privacy and Redaction

1. Cast device IDs must be redacted in evidence.
2. Trakt tokens must be redacted in evidence.
3. Network interface names may be partially redacted.
4. Resume points may contain sensitive timestamps.
5. Playback history may contain sensitive viewing patterns.