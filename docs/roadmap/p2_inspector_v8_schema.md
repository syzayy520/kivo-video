# P2 Inspector V8 Schema

Updated: 2026-06-26

Canonical schema name: `p2-inspector-v8`.

This document defines the required fields for Inspector snapshot evidence in V8 evidence files.

## Inspector Snapshot Structure

The Inspector snapshot is an optional field group in `p2-evidence-v8` schema. It provides a comprehensive snapshot of the playback pipeline state.

### Top-Level Inspector Fields

| Field | Type | Description |
|-------|------|-------------|
| `source_info` | object | Source identity and location information. |
| `probe_info` | object | FFmpeg probe results. |
| `demux_info` | object | Demultiplexer state and packet information. |
| `decode_info` | object | Decoder state and frame information. |
| `presenter_info` | object | Video presenter state. |
| `audio_info` | object | Audio output state. |
| `clock_info` | object | Master clock and AV sync state. |
| `seek_info` | object | Seek/flush/generation state. |
| `lifecycle_info` | object | Playback lifecycle state. |
| `threading_info` | object | Threading and COM ownership state. |
| `provider_info` | object | Provider-specific information (optional). |
| `extension_info` | object | Extension-specific information (optional). |
| `errors` | array | List of error entries. |

### Source Info Fields

```json
{
  "source_kind": "string (file, network, stream)",
  "protocol": "string (file, http, webdav, alist, emby, etc.)",
  "host": "string (redacted for privacy)",
  "path": "string (redacted for privacy)",
  "file_size": "number (bytes)",
  "is_local": "boolean",
  "is_readable": "boolean"
}
```

### Probe Info Fields

```json
{
  "format": "string (mov, mp4, mkv, etc.)",
  "duration_seconds": "number",
  "video_codec": "string (h264, hevc, etc.)",
  "audio_codec": "string (aac, mp3, etc.)",
  "video_dimensions": "string (1920x1080)",
  "audio_sample_rate": "number (44100, 48000)",
  "audio_channels": "number (2, 6, 8)",
  "probe_success": "boolean",
  "probe_time_ms": "number"
}
```

### Demux Info Fields

```json
{
  "packet_count": "number",
  "video_packet_count": "number",
  "audio_packet_count": "number",
  "keyframe_count": "number",
  "average_packet_size": "number",
  "max_packet_size": "number",
  "demux_success": "boolean",
  "demux_time_ms": "number"
}
```

### Decode Info Fields

```json
{
  "video_frames_decoded": "number",
  "audio_frames_decoded": "number",
  "video_drop_count": "number",
  "audio_drop_count": "number",
  "decode_error_count": "number",
  "average_decode_time_ms": "number",
  "max_decode_time_ms": "number",
  "decode_success": "boolean"
}
```

### Presenter Info Fields

```json
{
  "texture_created": "boolean",
  "first_frame_presented": "boolean",
  "first_frame_timestamp_ms": "number",
  "frame_count": "boolean",
  "drop_count": "number",
  "presenter_state": "string (idle, presenting, paused, stopped)",
  "device_type": "string (d3d11, warp, null)"
}
```

### Audio Info Fields

```json
{
  "endpoint_initialized": "boolean",
  "shared_mode": "boolean",
  "sample_rate": "number",
  "channels": "number",
  "bits_per_sample": "number",
  "buffer_size_frames": "number",
  "frames_written": "number",
  "audio_state": "string (stopped, playing, paused)"
}
```

### Clock Info Fields

```json
{
  "clock_initialized": "boolean",
  "master_clock_position_ms": "number",
  "video_pts_ms": "number",
  "audio_pts_ms": "number",
  "drift_ms": "number",
  "sync_policy": "string (video_master, audio_master, external)",
  "first_loop_complete": "boolean"
}
```

### Seek Info Fields

```json
{
  "seek_count": "number",
  "flush_count": "number",
  "generation": "number",
  "last_seek_target_ms": "number",
  "last_seek_success": "boolean",
  "flush_scope": "string (demux, decoder, both, none)"
}
```

### Lifecycle Info Fields

```json
{
  "current_state": "string (idle, preparing, ready, playing, paused, stopping, closed)",
  "transition_count": "number",
  "startup_complete": "boolean",
  "play_count": "number",
  "pause_count": "number",
  "stop_count": "number",
  "close_count": "number",
  "error_count": "number"
}
```

### Threading Info Fields

```json
{
  "com_initialization": "string (mta, sta, none)",
  "thread_ownership_recorded": "boolean",
  "d3d11_device_thread": "string (thread id)",
  "wasapi_thread": "string (thread id)",
  "ffmpeg_thread": "string (thread id)",
  "cross_thread_violations": "number",
  "thread_safety_guard_active": "boolean"
}
```

### Error Entry Fields

```json
{
  "error_id": "string",
  "domain": "string (source, probe, demux, decode, presenter, audio, clock, seek, lifecycle, threading)",
  "severity": "string (warning, error, fatal)",
  "message": "string (redacted)",
  "timestamp_ms": "number",
  "recoverable": "boolean",
  "recovery_action": "string (retry, skip, abort, fallback)"
}
```

## Usage in Evidence Files

Inspector snapshots are included in evidence files as an optional field group:

```json
{
  "schema_version": "p2-evidence-v8",
  "task_id": "LRD-008",
  "inspector_snapshot": {
    "source_info": { ... },
    "probe_info": { ... },
    "demux_info": { ... },
    "decode_info": { ... },
    "presenter_info": { ... },
    "audio_info": { ... },
    "clock_info": { ... },
    "seek_info": { ... },
    "lifecycle_info": { ... },
    "threading_info": { ... },
    "errors": [ ... ]
  }
}
```

## Validation Rules

1. All info objects must be present (can be empty objects).
2. `errors` must be an array (can be empty).
3. All timestamps must be in milliseconds.
4. All counts must be non-negative integers.
5. All state strings must be from allowed vocabularies.
6. No full paths, tokens, or credentials in any field.

## Example Evidence Files

See `artifacts/p2/evidence/LRD-008.json` and `artifacts/p2/evidence/LRD-013.json` for examples.