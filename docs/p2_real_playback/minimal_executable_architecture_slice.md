# P2-001A Minimal Executable Architecture Slice

Updated: 2026-06-25

## Goal

Define the thinnest vertical slice that produces a real video frame and audio sample on Windows, without expanding into full feature implementation.

## Slice Path

```
LocalFileSource
    → ByteStream (sequential read)
    → FFmpegProbe (format/container detection)
    → StreamInventory (video/audio/subtitle streams)
    → FFmpegDemux (packet extraction)
    → KivoPacket (typed packet wrapper)
    → FFmpegDecode (software decode)
    → DecodedFrame (YUV/RGB frame)
    → GraphNode (frame routing)
    → FrameQueue (bounded memory budget)
    → D3D11Presenter (texture upload + swap chain)
    → WASAPIEndpoint (shared-mode PCM)
    → PlaybackStateMachine (state transitions)
    → InspectorSnapshot (minimal diagnostics)
```

## Module Responsibilities

### 1. LocalFileSource

- **Responsibility**: Open a local file path, provide a ByteStream handle
- **Contract**: `SourcePlayableObject` (P1) extended with `LocalFileIdentity`
- **No**: network, credential, remote identity

### 2. ByteStream

- **Responsibility**: Sequential read with position tracking
- **Contract**: `ByteStreamContract { read, seek, position, length, close }`
- **No**: caching, range verification, network

### 3. FFmpegProbe

- **Responsibility**: Use `avformat_open_input` + `avformat_find_streams` to detect container format and stream inventory
- **Contract**: Produces `StreamInventory` (P1)
- **Adapter boundary**: FFmpeg types (`AVFormatContext`) stay inside adapter
- **No**: decode, render, network

### 4. FFmpegDemux

- **Responsibility**: Read packets from `AVFormatContext`, produce `KivoPacket`
- **Contract**: `DemuxRuntime { next_packet, seek, flush, close }`
- **KivoPacket**: `{ stream_index, pts, dts, duration, data_span, is_key_frame }`
- **No**: decode, render

### 5. FFmpegDecode

- **Responsibility**: Use `avcodec_send_packet` / `avcodec_receive_frame` for software decode
- **Contract**: `DecodeRuntime { send_packet, receive_frame, flush, close }`
- **Output**: `DecodedFrame { format, width, height, planes, pts }`
- **No**: render, network

### 6. GraphNode

- **Responsibility**: Route frames between producer and consumer with bounded queue
- **Contract**: `GraphNode { push, pop, try_pop, flush, drain, close }`
- **Invariant**: All edges have capacity, all nodes have lifecycle
- **No**: source ranking, policy

### 7. FrameQueue

- **Responsibility**: Bounded queue with memory budget enforcement
- **Contract**: `FrameQueue { push, pop, memory_usage, budget, is_full }`
- **Budget**: configurable max memory (e.g., 256MB for video frames)
- **No**: decode, render

### 8. D3D11Presenter

- **Responsibility**: Upload YUV frame to D3D11 texture, present via swap chain
- **Contract**: `PresenterService` (P1) extended with real D3D11
- **Adapter boundary**: D3D11/DXGI types stay inside adapter
- **No**: network, source

### 9. WASAPIEndpoint

- **Responsibility**: Shared-mode PCM audio output
- **Contract**: `AudioOutputDecision` (P1) extended with real WASAPI
- **Adapter boundary**: WASAPI types stay inside adapter
- **No**: video, network

### 10. PlaybackStateMachine

- **Responsibility**: Drive state transitions for play/pause/seek/stop
- **Contract**: `PlaybackState` (P1) with real transitions
- **Invariant**: Playing requires active session + graph binding
- **No**: decode, render

### 11. InspectorSnapshot

- **Responsibility**: Capture minimal playback diagnostics
- **Contract**: `PlaybackInspectorSnapshot` (P1) with real data
- **No**: network, credential

## Dependencies

| Module | Depends On | Adapter Boundary |
|--------|-----------|-----------------|
| LocalFileSource | Win32 API | Win32 handle in adapter |
| ByteStream | LocalFileSource | - |
| FFmpegProbe | ByteStream | AVFormatContext in adapter |
| FFmpegDemux | FFmpegProbe | AVFormatContext in adapter |
| FFmpegDecode | FFmpegDemux | AVCodecContext in adapter |
| GraphNode | FFmpegDecode | - |
| FrameQueue | GraphNode | - |
| D3D11Presenter | FrameQueue | D3D11/DXGI in adapter |
| WASAPIEndpoint | FFmpegDecode | WASAPI in adapter |
| PlaybackStateMachine | All above | - |
| InspectorSnapshot | All above | - |

## Files Created (P2-001A)

| File | Family | Responsibility |
|------|--------|----------------|
| `include/.../source_core/byte_stream_contract.hpp` | source_core | ByteStream contract |
| `include/.../source_core/local_file_identity.hpp` | source_core | Local file identity |
| `include/.../content_integrity_core/content_integrity_contract.hpp` | content_integrity_core | Content integrity |
| `include/.../demux_core/demux_runtime.hpp` | demux_core | Demux runtime contract |
| `include/.../demux_core/kivo_packet.hpp` | demux_core | Packet wrapper |
| `include/.../decode_core/decode_runtime.hpp` | decode_core | Decode runtime contract |
| `include/.../decode_core/decoded_frame.hpp` | decode_core | Decoded frame |
| `include/.../graph_core/graph_node.hpp` | graph_core | Graph node contract |
| `include/.../graph_core/graph_edge.hpp` | graph_core | Graph edge contract |
| `include/.../media_timeline_core/timebase_normalization.hpp` | media_timeline_core | Timebase |
| `include/.../media_timeline_core/master_clock.hpp` | media_timeline_core | Master clock |
| `include/.../media_timeline_core/av_sync_policy.hpp` | media_timeline_core | AV sync |
| `include/.../presentation_timing_core/frame_pacing.hpp` | presentation_timing_core | Frame pacing |
| `include/.../presentation_timing_core/vsync_policy.hpp` | presentation_timing_core | VSync |
| `include/.../video_render_core/d3d11_presenter.hpp` | video_render_core | D3D11 presenter |
| `include/.../audio_core/wasapi_endpoint.hpp` | audio_core | WASAPI endpoint |
| `include/.../resource_core/memory_budget.hpp` | resource_core | Memory budget |
| `include/.../storage_core/source_db.hpp` | storage_core | Source DB |
| `include/.../storage_core/playback_db.hpp` | storage_core | Playback DB |
| `include/.../storage_core/decision_db.hpp` | storage_core | Decision DB |

## Verification Gate

P2-001A is verified when:
1. All contract headers compile with MSVC C++20
2. Governance gate passes with new headers
3. No FFmpeg/D3D11/WASAPI types leak into core headers
4. `cmake --build --preset ninja-debug` succeeds
5. `ctest --preset ninja-debug --output-on-failure` passes
