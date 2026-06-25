# V10 Runtime Path Closure Plan

Created: 2026-06-25
Baseline: a44fae9 (P2-REAL-001 PASS)

This document defines the complete runtime path for local real playback closure, from file open to evidence generation.

---

## 1. Local File → Local Source Identity / Byte Stream Proof

**Step**: Open local file and establish stable byte stream.

**Components**:
- `LocalFileIdentity`: Redacted file path, size, modification time.
- `LocalByteStream`: Open, read, seek operations on local file.

**Proof**:
- Byte stream open returns valid handle.
- Read returns actual file bytes.
- Seek moves to correct position.
- No full path leak in logs/inspector.

**Evidence**: `playback_proof.json` → `source.open` field.

---

## 2. FFmpeg Probe → Stream Selection

**Step**: Probe media container for streams.

**Components**:
- `RealProbeRuntime`: FFmpeg `avformat_open_input` + `avformat_find_stream_info`.

**Proof**:
- Probe returns container format, duration, video/audio stream count.
- Stream selection picks best video/audio streams.
- No fake probe (must use real FFmpeg).

**Evidence**: `playback_proof.json` → `probe` field.

---

## 3. Demux Packets → KivoPacket.data

**Step**: Read compressed packets from container.

**Components**:
- `RealDemuxRuntime`: FFmpeg `av_read_frame` → `KivoPacket.data` (real compressed bytes).

**Proof**:
- `KivoPacket.data` contains actual compressed packet payload.
- `packet.size` matches actual data size.
- No empty packet or metadata-only packet.

**Evidence**: `playback_proof.json` → `demux` field.

---

## 4. Software Decode → Decoded Video/Audio Frames

**Step**: Decode compressed packets into raw frames.

**Components**:
- `RealSoftwareDecodeRuntime`: FFmpeg `avcodec_send_packet` + `avcodec_receive_frame`.

**Proof**:
- Video: width, height, pixel_format are real (e.g., 320x240 yuv420p).
- Audio: sample_rate, channels, sample_format are real (e.g., 44100Hz 1ch fltp).
- No `send_packet(nullptr, 0)`.
- No fake decode.

**Evidence**: `playback_proof.json` → `decode` field.

---

## 5. D3D11 First Decoded Frame Upload

**Step**: Upload decoded video frame to GPU texture.

**Components**:
- `DecodedVideoFrameUploader`: Convert DecodedFrame to D3D11 texture.
- `D3D11DeviceContext`: Create device/context (hardware or WARP).
- `D3D11TextureUpload`: Create texture and upload pixel data.

**Proof**:
- Texture creation succeeds.
- Upload returns actual width/height/format.
- No empty texture upload.
- WARP fallback detected and reported.

**Evidence**: `playback_proof.json` → `video_upload` field.

---

## 6. Presenter Boundary First Frame

**Step**: Presenter receives uploaded texture.

**Components**:
- `PresenterBoundaryShell`: Receive texture handle, record first-frame proof.

**Proof**:
- First frame received within timeout.
- Texture dimensions match decoded frame.
- No fake presenter.

**Evidence**: `playback_proof.json` → `presenter` field.

---

## 7. PCM Conversion → WASAPI Shared First Buffer

**Step**: Convert decoded audio to PCM and write to WASAPI.

**Components**:
- `DecodedAudioFrameConverter`: Convert DecodedFrame to PCM buffer.
- `WasapiSharedPcmWriter`: Open WASAPI shared mode, write first buffer.

**Proof**:
- PCM buffer contains actual audio samples.
- WASAPI write returns success.
- No silence-buffer fake.
- WASAPI endpoint detected and reported.

**Evidence**: `playback_proof.json` → `audio_output` field.

---

## 8. Master Clock → AV Sync Decision

**Step**: Synchronize audio/video using master clock.

**Components**:
- `MasterClock`: Audio master clock snapshot.
- `AvSyncPolicy`: First AV sync scheduling decision.

**Proof**:
- Clock snapshot contains real timestamp.
- Sync decision recorded (video ahead/behind/audio master).
- No fake clock.

**Evidence**: `playback_proof.json` → `clock` field.

---

## 9. Seek / Flush / Generation Isolation

**Step**: Seek operation with flush and generation tracking.

**Components**:
- Seek request → flush decode buffers → increment generation.

**Proof**:
- Seek to position N.
- Flush proof (decode buffers cleared).
- Generation start/end recorded.
- No generation leak.

**Evidence**: `playback_proof.json` → `seek` field.

---

## 10. Pause / Resume / Stop / Close Lifecycle

**Step**: Lifecycle transitions with resource release.

**Components**:
- `PlaybackSessionController`: State machine transitions.

**Proof**:
- Pause: decode stops, clock pauses.
- Resume: decode resumes, clock resumes.
- Stop: all threads join, resources released.
- Close: all handles closed, memory freed.

**Evidence**: `playback_proof.json` → `lifecycle` field.

---

## 11. Threading / COM / Device Ownership Proof

**Step**: Document thread ownership and COM apartment model.

**Components**:
- FFmpeg context: Demux/decode thread, no COM.
- D3D11 device: Main/render thread, MTA.
- WASAPI client: Audio thread, MTA.

**Proof**:
- No hot-path diagnostic/source lock waits.
- All resources released on close.
- COM apartment model documented.

**Evidence**: `playback_proof.json` → `threading` field.

---

## 12. Inspector / DecisionLedger / ErrorMapping

**Step**: Record all decisions and errors.

**Components**:
- `PlaybackInspectorSnapshot`: Snapshot of all key decisions.
- `DecisionLedgerEntry`: Decision chain for replay.
- Error mapping: FFmpeg/D3D11/WASAPI errors to EngineError.

**Proof**:
- Inspector snapshot contains real path.
- DecisionLedger records no fake fallback.
- Error mapping covers all failure points.

**Evidence**: `inspector_snapshot.json`, `decision_ledger.json`.

---

## 13. Machine-Readable Evidence

**Step**: Generate JSON evidence files.

**Components**:
- `playback_proof.json`: End-to-end playback proof.
- `inspector_snapshot.json`: Inspector state at completion.
- `decision_ledger.json`: All decisions made during playback.
- `micro_soak_metrics.json`: Queue bounds, memory, timing (for 30s soak).
- `ffmpeg_off_antifake.json`: FFmpeg OFF anti-fake proof.
- `evidence_schema_validation.json`: Schema validation result.

**Proof**:
- All files generated in `docs/v10_evidence/`.
- All files schema-valid.
- No sensitive data in evidence.

**Evidence**: `docs/v10_evidence/` directory.

---

## 14. Runtime Path Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│ Local File                                                      │
│   ↓                                                             │
│ LocalByteStream (open, read, seek)                              │
│   ↓                                                             │
│ RealProbeRuntime (FFmpeg probe)                                 │
│   ↓                                                             │
│ RealDemuxRuntime (FFmpeg demux → KivoPacket.data)               │
│   ↓                                                             │
│ RealSoftwareDecodeRuntime (FFmpeg decode → DecodedFrame)        │
│   ↓                                                             │
│ ┌─────────────────────┬─────────────────────┐                   │
│ │ Video Path          │ Audio Path          │                   │
│ │ ↓                   │ ↓                   │                   │
│ │ DecodedVideoFrame   │ DecodedAudioFrame   │                   │
│ │ Uploader            │ Converter           │                   │
│ │ ↓                   │ ↓                   │                   │
│ │ D3D11TextureUpload  │ WasapiSharedPcm     │                   │
│ │ ↓                   │ Writer              │                   │
│ │ PresenterBoundary   │ ↓                   │                   │
│ │ Shell               │ Audio Output        │                   │
│ └─────────────────────┴─────────────────────┘                   │
│   ↓                                                             │
│ MasterClock → AvSyncPolicy                                      │
│   ↓                                                             │
│ PlaybackSessionController (lifecycle)                           │
│   ↓                                                             │
│ PlaybackInspectorSnapshot + DecisionLedgerEntry                 │
│   ↓                                                             │
│ Machine-Readable Evidence (JSON)                                │
└─────────────────────────────────────────────────────────────────┘
```

---

## 15. Implementation Order

1. Local source identity / byte stream (existing)
2. FFmpeg probe (existing)
3. FFmpeg demux (existing)
4. FFmpeg decode (existing)
5. D3D11 first frame upload (NEW)
6. Presenter boundary (NEW)
7. PCM conversion / WASAPI shared (NEW)
8. Master clock / AV sync (existing)
9. Seek/flush/generation (NEW)
10. Lifecycle (NEW)
11. Threading/COM ownership (NEW)
12. Inspector/DecisionLedger/ErrorMapping (NEW)
13. Machine-readable evidence (NEW)

---

## 16. Anti-Fake Proof Points

| Step | Anti-Fake Requirement |
|------|----------------------|
| Source | Real file bytes read, not empty buffer |
| Probe | Real FFmpeg probe, not fake |
| Demux | Real compressed packet in KivoPacket.data |
| Decode | Real frame dimensions/formats, not null |
| Video Upload | Real texture upload, not empty texture |
| Presenter | Real first frame received |
| Audio | Real PCM samples, not silence |
| Clock | Real timestamp, not fake |
| Seek | Real flush proof |
| Lifecycle | Real resource release |
| Threading | Real thread ownership |
| Inspector | Real decision recording |

---

## 17. Evidence Schema

All evidence files must follow schema in `docs/v10_evidence/evidence_schema.json`.

---

## 18. Validation Commands

```bash
# FFmpeg ON build
cmake --preset vs-debug-ffmpeg
cmake --build --preset vs-debug-ffmpeg

# Run all V10 tests
ctest --preset vs-debug-ffmpeg --output-on-failure

# Generate evidence
kivo_local_real_playback_integration_test --generate-evidence
kivo_local_playback_micro_soak_test --generate-evidence

# Validate evidence
kivo_evidence_schema_validator --input docs/v10_evidence/
```

---

## 19. Known Limitations

- D3D11 may be unavailable in CI/headless (WARP fallback).
- WASAPI may be unavailable in CI/headless (loopback/silence fallback).
- 30s micro soak requires real sample file.
- FFmpeg OFF mode disables all real playback tests.

---

## 20. Next Recommendation

After V10-005 completion, proceed to V10-006 Runtime Environment Gate to verify all runtime components are available.