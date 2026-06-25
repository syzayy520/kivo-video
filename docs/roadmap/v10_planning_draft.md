# P2-REAL-LOCAL-CLOSURE-V10 Planning Draft

Created: 2026-06-25
Baseline commit: a44fae9 (P2-REAL-001 PASS)

---

## 1. Current Tree Inventory

### Production modules (backend/include/kivo/cinema_engine/)
- `decode_core/` — 10 headers (real_software_decode_runtime, decoded_frame, decode_result, etc.)
- `demux_core/` — 12 headers (real_demux_runtime, real_probe_runtime, kivo_packet, etc.)
- `video_render_core/` — 12 headers (d3d11_presenter, video_presenter_contract, fake_presenter_service, etc.)
- `audio_core/` — 4 headers (audio_endpoint_contract, audio_output_decision, fake services)
- `presentation_timing_core/` — 2 headers (frame_pacing, vsync_policy)
- `media_timeline_core/` — 5 headers (master_clock, timebase_normalization, av_sync_policy, etc.)
- `graph_core/` — graph node/edge
- `playback_core/` — state machine, playback state
- `playback_inspector_core/` — inspector runtime
- `decision_ledger_core/` — decision ledger
- `third_party_adapter/ffmpeg_adapter/` — ffmpeg_adapter.hpp/cpp (adapter boundary)

### Key V10 runtime files (already exist)
- `real_demux_runtime.hpp/cpp` — FFmpeg demux with KivoPacket.data
- `real_software_decode_runtime.hpp/cpp` — FFmpeg decode with adopt_codec_context
- `real_probe_runtime.hpp/cpp` — FFmpeg probe
- `ffmpeg_adapter.hpp/cpp` — FFmpeg adapter boundary (opaque handles)
- `d3d11_presenter.hpp` — PresenterRuntimeContract
- `audio_endpoint_contract.hpp` — AudioEndpointRuntimeContract
- `master_clock.hpp` — MasterClock
- `decoded_frame.hpp` — DecodedFrame struct

---

## 2. V10 Natural Family Tree

```
source/
  local_file_identity
  local_byte_stream

probe/
  real_probe_runtime

demux/
  real_demux_runtime
  kivo_packet

decode/
  real_software_decode_runtime
  decoded_frame

video_upload/
  decoded_video_frame_uploader    (NEW)
  yuv_rgb_conversion              (NEW)
  d3d11_device_context            (NEW)
  d3d11_texture_upload            (NEW)

audio_output/
  decoded_audio_frame_converter   (NEW)
  wasapi_shared_pcm_writer        (NEW)

clock/
  master_clock
  av_sync_policy

graph/
  playback_graph                  (NEW minimal)

presenter/
  presenter_boundary_shell        (NEW)

lifecycle/
  playback_session_controller     (NEW)

inspector/
  playback_inspector_snapshot     (NEW)
  decision_ledger_entry           (NEW)

test/
  local_real_playback_integration (NEW)
  local_playback_micro_soak       (NEW)
  failure_injection               (NEW)
```

---

## 3. Allowed Files

### Production (V10 new code)
- `backend/include/kivo/cinema_engine/video_upload/` — decoded_video_frame_uploader.hpp, yuv_rgb_conversion.hpp, d3d11_device_context.hpp, d3d11_texture_upload.hpp
- `backend/include/kivo/cinema_engine/audio_output/` — decoded_audio_frame_converter.hpp, wasapi_shared_pcm_writer.hpp
- `backend/include/kivo/cinema_engine/graph/playback_graph.hpp`
- `backend/include/kivo/cinema_engine/presenter/presenter_boundary_shell.hpp`
- `backend/include/kivo/cinema_engine/lifecycle/playback_session_controller.hpp`
- `backend/include/kivo/cinema_engine/inspector/playback_inspector_snapshot.hpp`

### Test (V10 new tests)
- `backend/tests/integration_local/local_real_playback_integration_test.cpp`
- `backend/tests/integration_local/local_playback_micro_soak_test.cpp`
- `backend/tests/integration_local/failure_injection_test.cpp`

### Modified (existing files)
- `backend/src/decode_core/real_software_decode_runtime.cpp` — minor adjustments
- `backend/src/demux_core/real_demux_runtime.cpp` — minor adjustments
- `backend/CMakeLists.txt` — register new targets
- `CMakePresets.json` — add V10 presets

---

## 4. Forbidden Files

- Any P3 code (hardware decode, HDR output, exclusive audio, passthrough)
- Any remote code (WebDAV, AList, Emby, SMB, NFS)
- Any UI code (Qt, QML, media library)
- Any BDMV/ISO code
- Any new fake service code (V10 uses real runtime only)

---

## 5. Dependency Direction

```
video_upload → decode (DecodedFrame)
video_upload → third_party_adapter (D3D11 opaque handle)
audio_output → decode (DecodedFrame)
audio_output → third_party_adapter (WASAPI opaque handle)
graph → decode, demux, clock
presenter → video_upload
lifecycle → graph, presenter, audio_output, inspector
inspector ← all modules (read-only observer)
```

No circular dependencies. All D3D11/WASAPI types stay inside adapter boundary.

---

## 6. Single-File Responsibility Table

| File | Responsibility |
|------|---------------|
| decoded_video_frame_uploader.hpp | Interface: upload DecodedFrame to GPU texture |
| yuv_rgb_conversion.hpp | YUV→RGB/NV12 pixel format conversion policy |
| d3d11_device_context.hpp | D3D11 device/context lifecycle (opaque) |
| d3d11_texture_upload.hpp | Texture creation + upload proof |
| decoded_audio_frame_converter.hpp | DecodedFrame→PCM format conversion |
| wasapi_shared_pcm_writer.hpp | WASAPI shared mode PCM buffer write |
| playback_graph.hpp | Minimal graph: demux→decode→output nodes |
| presenter_boundary_shell.hpp | Presenter boundary: receive texture, record proof |
| playback_session_controller.hpp | Lifecycle: open→play→pause→resume→seek→stop→close |
| playback_inspector_snapshot.hpp | Inspector snapshot: all key decisions |

---

## 7. Test Family Plan

| Test | Type | Scope |
|------|------|-------|
| kivo_real_demux_decode_integration_test | Unit/Integration | P2-REAL-001 preservation |
| kivo_d3d11_first_frame_test | Integration | D3D11 texture upload proof |
| kivo_wasapi_shared_first_buffer_test | Integration | WASAPI shared PCM proof |
| kivo_clock_av_sync_test | Unit | Master clock + AV sync |
| kivo_local_real_playback_integration_test | Integration | Full end-to-end local playback |
| kivo_local_playback_micro_soak_test | Integration | 30s bounded soak |
| kivo_p2_failure_injection_test | Integration | Failure injection coverage |

---

## 8. Runtime Environment Risk

| Risk | Mitigation |
|------|-----------|
| D3D11 device unavailable (CI/headless) | Use WARP adapter as fallback; test must detect and report |
| WASAPI device unavailable (CI/headless) | Use loopback or silence device; test must detect and report |
| FFmpeg ON build fails | BLOCKED condition; do not proceed |
| No real sample available | BLOCKED condition; sample gate required |
| COM apartment model issues | Use MTA for D3D11/WASAPI; document in threading plan |

---

## 9. Sample Availability Risk

| Sample | Status | Path |
|--------|--------|------|
| H.264/AAC MP4 (320x240, 1s) | EXISTS | tests/real/test_sample.mp4 |
| H.264/AAC MP4 (30s+) for soak | NEEDED | Must create for V10-021 |
| H.264/AC3 MKV (optional) | NOT YET | Optional for V10-012 |

---

## 10. STOP / BLOCKED / FAILED Conditions

- **STOP_BRANCH_BASE_INVALID**: Branch or base commit mismatch
- **STOP_WORKSPACE_DIRTY**: Unexpected code changes in workspace
- **BLOCKED_SAMPLE_MISSING**: No real sample file available
- **BLOCKED_SAMPLE_TOO_SHORT**: Sample < 30s for micro soak
- **BLOCKED_RUNTIME_ENV**: D3D11/WASAPI/FFmpeg unavailable
- **FAILED_SCOPE_LEAK**: P3/Remote/UI terms found in new code
- **FAILED_REAL_RUNTIME**: Fake runtime used instead of real
- **FAILED_ANTI_FAKE**: open decoder counted as decode, read packet counted as frame

---

## 11. Anti-Fake Proof Strategy

Every V10 task must prove:
1. **Real input**: actual file bytes, actual compressed packets
2. **Real runtime**: actual FFmpeg decode, actual D3D11 API call, actual WASAPI API call
3. **Real output**: actual decoded frame dimensions, actual texture upload, actual PCM samples written
4. **Real assertion**: assert on output properties, not just success flag
5. **Inspector evidence**: Inspector snapshot records the real path
6. **Scope proof**: no forbidden terms in new code

Forbidden substitutions:
- open decoder ≠ decode
- read packet ≠ decoded frame
- fake probe ≠ real probe
- FFmpeg OFF ≠ FFmpeg ON
- silence buffer ≠ real PCM
- empty texture ≠ real upload

---

## 12. Machine-Readable Evidence Plan

Artifacts to generate:
- `playback_proof.json` — end-to-end playback proof
- `inspector_snapshot.json` — Inspector state at completion
- `decision_ledger.json` — all decisions made during playback
- `micro_soak_metrics.json` — queue bounds, memory, timing
- `ffmpeg_off_antifake.json` — FFmpeg OFF anti-fake proof
- `evidence_schema_validation.json` — schema validation result

---

## 13. Artifact Path Plan

All V10 artifacts go to:
```
docs/v10_evidence/
  playback_proof.json
  inspector_snapshot.json
  decision_ledger.json
  micro_soak_metrics.json
  ffmpeg_off_antifake.json
  evidence_schema_validation.json
```

---

## 14. CMake Registration Plan

New CMake targets:
- `kivo_d3d11_first_frame_test` (executable, links kivo_cinema_engine)
- `kivo_wasapi_shared_first_buffer_test` (executable)
- `kivo_clock_av_sync_test` (executable)
- `kivo_local_real_playback_integration_test` (executable)
- `kivo_local_playback_micro_soak_test` (executable)
- `kivo_p2_failure_injection_test` (executable)

V10 CMake preset:
- `vs-debug-ffmpeg-v10` — MSVC x64 Debug, KIVO_ENABLE_FFMPEG=ON

---

## 15. Scope Leak Scan Plan

Before commit, run forbidden token scan on all new/modified files:
- Pattern: `D3D11|DXGI|Vulkan|Metal|OpenGL|Win32|COM|HWND|HANDLE|void\*|uintptr_t|AVFrame|AVPacket|AVBufferRef|FFmpeg|swapchain|shader`
- Exception: `ffmpeg_adapter.cpp/hpp` (adapter boundary, allowed)
- Exception: comments in documentation files
- Any match in new production code = FAILED_SCOPE_LEAK

---

## 16. Diff Base Plan

- Base commit: `a44fae9` (P2-REAL-001 PASS)
- All V10 changes diff from this base
- Final diff must show only V10-allowed files

---

## 17. Threading / COM / Device Ownership Plan

| Resource | Thread | COM Model | Ownership |
|----------|--------|-----------|-----------|
| FFmpeg context | Demux/decode thread | None (no COM) | RealDemuxRuntime/RealSoftwareDecodeRuntime |
| D3D11 device | Main/render thread | MTA | d3d11_device_context |
| D3D11 texture | Main/render thread | MTA | d3d11_texture_upload |
| WASAPI client | Audio thread | MTA | wasapi_shared_pcm_writer |
| Master clock | Any (thread-safe) | None | master_clock |

No hot-path diagnostic/source lock waits. All resources released on close.

---

## 18. Failure Injection Plan

| Failure | Injection Point | Expected Behavior |
|---------|----------------|-------------------|
| FFmpeg decode failure | send_packet returns error | Error mapped, playback stops |
| D3D11 device lost | device_context returns error | Error mapped, presenter stops |
| WASAPI buffer full | write returns AUDCLNT_BUFFER_CAPACITY_SUFFICIENT | Retry/backpressure |
| Clock drift > threshold | av_sync_policy detects | Frame drop/insert decision |
| Seek during playback | seek_request | Flush + generation increment |
| Close during decode | close() called | Graceful shutdown, no leak |

---

## 19. Resource Release Proof Plan

On close/stop:
- FFmpeg: avcodec_free_context, avformat_close_input
- D3D11: Release() on texture, device, context
- WASAPI: Release() on audio client, render client
- Threads: join all spawned threads
- Queues: clear all queues

Proof: assert all pointers null after close, assert no handles open.

---

## 20. Final Self-Check Checklist

- [ ] Branch is `kivo-video-p2-real-001-repair-ffmpeg-proof`
- [ ] Base is `a44fae9`
- [ ] No code changes outside V10 allowed files
- [ ] No forbidden tokens in new code
- [ ] All new tests compile and pass
- [ ] Real sample available for all tests
- [ ] D3D11/WASAPI availability detected at runtime
- [ ] Inspector records real path for every operation
- [ ] DecisionLedger records no fake fallback
- [ ] All resources released on close
- [ ] FFmpeg ON build passes
- [ ] FFmpeg OFF anti-fake passes
- [ ] Machine-readable evidence generated
- [ ] Final report classification correct
