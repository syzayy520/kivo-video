# Trace Integration Guide

## Overview

The real media processing pipeline integrates with the Kivo Video tracing system through `ContractMetadata::trace_id` propagation. Each operation in the vertical slice receives and propagates a trace ID for end-to-end observability.

## Trace Propagation Flow

```
User Request
    │
    ▼
trace_id = "playback_001"
    │
    ├─► LocalFileByteStream::open(request)
    │       request.trace_id = "playback_001"
    │       result.trace_id  = "playback_001"
    │
    ├─► RealProbeRuntime::probe_file(path, "playback_001")
    │       result.trace_id  = "playback_001"
    │
    ├─► RealDemuxRuntime::open(path, "playback_001")
    │       error_code / inspector_hint recorded
    │
    ├─► RealDemuxRuntime::read_packet("playback_001")
    │       result.trace_id  = "playback_001"
    │       result.packet.packet_id = "pkt_0_12345"
    │
    ├─► RealSoftwareDecodeRuntime::open("h264", "playback_001")
    │       error_code / inspector_hint recorded
    │
    └─► RealSoftwareDecodeRuntime::decode(packet, "playback_001")
            result.frame.frame_id = "frame_12345"
```

## Inspector Hints

Each operation provides an `inspector_hint` string that describes the operation result in human-readable form:

| Operation | Success Hint | Failure Hint |
|-----------|-------------|--------------|
| ByteStream::open | "Successfully opened file: {path}" | "File does not exist: {path}" |
| Probe::probe_file | (set in ProbeResult) | "Failed to open file: {path}" |
| Demux::open | "Demuxer opened: {path} [{format}]" | "Failed to open file for demuxing: {path}" |
| Demux::read_packet | "Packet read OK" | "End of stream" or error code |
| Decode::open | "Decoder opened: {codec}" | "Decoder not found: {codec}" |
| Decode::decode | "Frame decoded OK" | Error-specific message |
| Decode::flush | "Decoder flushed successfully" | "Error flushing decoder" |

## Error Codes

Standardized error codes for the real media pipeline:

| Error Code | Source | Meaning |
|------------|--------|---------|
| `file_not_found` | ByteStream | File does not exist |
| `permission_denied` | ByteStream | Cannot open file |
| `open_failed` | Probe/Demux | FFmpeg open_file failed |
| `probe_failed` | Probe | Container metadata extraction failed |
| `ffmpeg_not_available` | All | FFmpeg disabled at build time |
| `not_open` | Demux/Decode | Operation on closed runtime |
| `codec_not_found` | Decode | Decoder not available |
| `context_alloc_failed` | Decode | Codec context allocation failed |
| `packet_alloc_failed` | Demux/Decode | Packet allocation failed |
| `stream_not_open` | Probe | Byte stream not open |
| `stream_probe_not_supported` | Probe | Stream-based probing not implemented |

## Decision Ledger Integration

The real media pipeline records decisions to the DecisionLedger:

```cpp
// After successful probe
DecisionLedger::record({
    .trace_id = trace_id,
    .decision = "probe_success",
    .details = {
        .container_format = result.container_format,
        .duration = result.duration,
        .video_streams = result.video_streams.size(),
        .audio_streams = result.audio_streams.size()
    }
});

// After successful decode
DecisionLedger::record({
    .trace_id = trace_id,
    .decision = "decode_frame",
    .details = {
        .codec = codec_name,
        .width = width_,
        .height = height_,
        .pixel_format = pixel_format_
    }
});
```

## Telemetry Events

Key telemetry events for monitoring:

| Event | When | Payload |
|-------|------|---------|
| `media.probe.start` | Before probe | `{path, trace_id}` |
| `media.probe.end` | After probe | `{success, container_format, duration, stream_counts}` |
| `media.demux.open` | After demux open | `{success, container_format, stream_counts}` |
| `media.demux.packet` | After each packet | `{stream_index, pts, is_key_frame, data_size}` |
| `media.demux.eof` | End of stream | `{total_packets, trace_id}` |
| `media.decode.open` | After decoder open | `{success, codec_name, width, height}` |
| `media.decode.frame` | After each frame | `{frame_id, stream_kind, pts, width, height}` |
| `media.decode.flush` | After flush | `{codec_name, frames_drained}` |
