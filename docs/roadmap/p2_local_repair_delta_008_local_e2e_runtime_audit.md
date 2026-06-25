# LRD-008 Local E2E Runtime Audit

## 1. Task Overview

**Task ID**: LRD-008  
**Task Family**: local_repair_delta/integration  
**Depends On**: LRD-007  
**Goal**: Local playback E2E touches real source, probe, demux, decode, upload/presenter boundary, PCM/WASAPI boundary, clock, seek, lifecycle; not state-machine-only.

## 2. Current Status Analysis

**Current Implementation**: `local_playback_e2e_test.cpp`  
**Current Status**: RUNTIME_PASS (real FFmpeg pipeline)  
**Evidence**: V10-020 passes with real FFmpeg probe/demux/decode

**Current E2E Pipeline**:
1. **Probe**: Real FFmpeg probe of local file
2. **Demux**: Real FFmpeg demux of container
3. **Decode**: Real FFmpeg software decode
4. **Video Upload**: Real D3D11 texture upload (when available)
5. **Audio Conversion**: Real PCM conversion
6. **Audio Output**: Real WASAPI shared mode (when available)

**Current Test Coverage**: 12 tests in `local_playback_e2e_test.cpp`

## 3. Reality Proof Requirements

### 3.1 Real Source Interaction
- **Real Local File**: Must use actual local file path
- **Real File Reading**: Must read actual file bytes
- **Real Format Detection**: Must detect actual container format

### 3.2 Real Probe Pipeline
- **Real FFmpeg Probe**: Must call real `avformat_open_input` and `avformat_find_stream_info`
- **Real Stream Detection**: Must detect actual video/audio streams
- **Real Metadata**: Must extract real duration, codec, format info

### 3.3 Real Demux Pipeline
- **Real FFmpeg Demux**: Must call real `av_read_frame`
- **Real Packet Data**: Must read actual compressed packet data
- **Real Stream Selection**: Must select actual video/audio streams

### 3.4 Real Decode Pipeline
- **Real FFmpeg Decode**: Must call real `avcodec_send_packet` and `avcodec_receive_frame`
- **Real Frame Data**: Must decode actual pixel/sample data
- **Real Frame Metadata**: Must extract real PTS, dimensions, format

### 3.5 Real Upload/Presenter Boundary
- **Real D3D11 Upload**: When D3D11 available, must upload real frame data
- **Real Texture Creation**: Must create real texture with actual dimensions
- **Real Presenter**: Must present real texture (not just record timestamp)

### 3.6 Real PCM/WASAPI Boundary
- **Real PCM Conversion**: Must convert real audio samples
- **Real WASAPI Write**: When WASAPI available, must write real PCM data
- **Real Audio Output**: Must start/stop real audio playback

### 3.7 Real Clock/Seek/Lifecycle
- **Real Clock**: Must track real playback time
- **Real Seek**: Must perform real seek operation
- **Real Lifecycle**: Must follow real startup/play/pause/stop/close sequence

## 4. Allowed/Forbidden Files

### 4.1 Allowed Production Files
- `backend/tests/presenter/local_playback_e2e_test.cpp`
- `backend/include/kivo/cinema_engine/demux_core/real_demux_runtime.hpp`
- `backend/src/demux_core/real_demux_runtime.cpp`
- `backend/include/kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp`
- `backend/src/decode_core/real_software_decode_runtime.cpp`
- `backend/include/kivo/cinema_engine/demux_core/real_probe_runtime.hpp`
- `backend/src/demux_core/real_probe_runtime.cpp`
- `backend/include/kivo/cinema_engine/video_upload/d3d11_device_context.hpp`
- `backend/src/video_upload/d3d11_device_context.cpp`
- `backend/include/kivo/cinema_engine/video_upload/d3d11_texture_upload.hpp`
- `backend/src/video_upload/d3d11_texture_upload.cpp`
- `backend/include/kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp`
- `backend/src/audio_output/decoded_audio_frame_converter.cpp`
- `backend/include/kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp`
- `backend/src/audio_output/wasapi_shared_pcm_writer.cpp`

### 4.2 Allowed Test Files
- `backend/tests/presenter/local_playback_e2e_test.cpp`

### 4.3 Forbidden Files
- No new families or directories
- No modifications to non-E2E families
- No additions of state machines or simulators

## 5. Dependency Direction

```
LocalPlaybackE2E
  ↑
RealProbeRuntime (file probing)
  ↑
RealDemuxRuntime (container demux)
  ↑
RealSoftwareDecodeRuntime (frame decode)
  ↑
D3d11DeviceContext + D3d11TextureUpload (video upload)
  ↑
DecodedAudioFrameConverter (audio conversion)
  ↑
WasapiSharedPcmWriter (audio output)
```

**Direction**: E2E test depends on all pipeline components. No reverse dependencies.

## 6. Single-File Responsibility Table

| File | Responsibility | Max Lines |
|------|---------------|-----------|
| local_playback_e2e_test.cpp | E2E runtime proof | ≤300 |
| real_demux_runtime.hpp | Real FFmpeg demux interface | ≤100 |
| real_demux_runtime.cpp | Real FFmpeg demux implementation | ≤200 |
| real_software_decode_runtime.hpp | Real FFmpeg decode interface | ≤100 |
| real_software_decode_runtime.cpp | Real FFmpeg decode implementation | ≤200 |

## 7. Test Family Plan

### 7.1 Required Reality Tests
1. **Real Source Reading Test**: Read actual local file bytes
2. **Real Probe Test**: Call real FFmpeg probe functions
3. **Real Demux Test**: Call real FFmpeg demux functions
4. **Real Decode Test**: Call real FFmpeg decode functions
5. **Real Video Upload Test**: Upload real frame data to D3D11 (when available)
6. **Real Audio Conversion Test**: Convert real audio samples to PCM
7. **Real WASAPI Write Test**: Write real PCM data to WASAPI (when available)
8. **Real Clock Test**: Track real playback time
9. **Real Seek Test**: Perform real seek operation
10. **Real Lifecycle Test**: Follow real startup/play/pause/stop/close sequence

### 7.2 Test Requirements
- All tests must use real FFmpeg functions (not stubs)
- Tests must handle device unavailable scenarios honestly
- Evidence must prove real runtime, not just state machine
- Platform: Windows (for D3D11/WASAPI), cross-platform (for FFmpeg)

## 8. Runtime Environment Risk

**Risk**: D3D11/WASAPI may be unavailable on non-Windows or headless systems  
**Mitigation**:
- Detect device availability before tests
- Skip device-specific tests when unavailable
- Focus on FFmpeg pipeline (cross-platform)
- Document device requirements clearly

## 9. Sample Availability Risk

**Risk**: No sample video/audio file available  
**Mitigation**:
- Use existing 320x240 H.264 + 44100Hz AAC sample
- Verify sample exists before tests
- Document sample requirements
- Handle missing sample gracefully

## 10. STOP/BLOCKED/FAILED Conditions

### STOP Conditions
- Sample file not found
- FFmpeg not available
- Real probe fails
- Real demux fails
- Real decode fails
- Real upload fails (when D3D11 available)
- Real audio conversion fails
- Real WASAPI write fails (when WASAPI available)

### BLOCKED Conditions
- No sample file → `BLOCKED_ENV`
- FFmpeg not compiled → `BLOCKED_ENV`
- D3D11 not available → video upload skipped (not blocked)
- WASAPI not available → audio output skipped (not blocked)

### FAILED Conditions
- State machine only → `CONTRACT_PASS`
- Stub FFmpeg functions → `CONTRACT_PASS`
- Simulated demux/decode → `CONTRACT_PASS`
- No real frame data → `CONTRACT_PASS`

## 11. Anti-Fake Proof Strategy

### 11.1 Real E2E Runtime Proof
- Must show real file reading
- Must show real FFmpeg function calls
- Must show real frame data (non-empty frame_data vector)
- Must show real packet data (non-empty packet data)
- Must show real upload (when D3D11 available)
- Must show real audio output (when WASAPI available)

### 11.2 Forbidden Fake Proofs
- State machine simulation
- Stub FFmpeg functions
- Mock demux/decode
- Simulated frame data
- Interface contract only

## 12. Machine-Readable Evidence Plan

### 12.1 Evidence File
- Path: `artifacts/p2/evidence/LRD-008.json`
- Schema: `p2-evidence-v8`
- Required fields: task_id, status, runtime_mode, runtime_environment, e2e_pipeline, source_reading, probe_result, demux_result, decode_result, video_upload, audio_conversion, audio_output, clock, seek, lifecycle, inspector_snapshot, decision_ledger, errors, redaction, schema_validation, anti_fake, redline

### 12.2 Inspector Snapshot Requirements
- E2E pipeline snapshot: source, probe, demux, decode, upload, conversion, output, clock, seek, lifecycle

### 12.3 Decision Ledger Requirements
- E2E decision: input (file path), output (success, frames_decoded, frames_uploaded, samples_converted)

## 13. CMake Registration Plan

### 13.1 Test Registration
- Add `kivo_local_e2e_runtime_test` to CMakeLists.txt
- Register under `backend/tests/presenter/`
- Add to CTest with appropriate timeout (60s)
- Conditional on FFmpeg availability

### 13.2 Build Verification
- Ensure E2E test builds correctly
- Verify FFmpeg links correctly
- Confirm test runs on all platforms

## 14. Scope Leak Scan Plan

### 14.1 Forbidden Terms
- Scan for: P3, remote, HDR, hardware decode, exclusive, passthrough, BDMV, ISO
- Scan for: state machine, simulator, stub FFmpeg, mock demux

### 14.2 Allowed Terms
- Local playback, real FFmpeg, real demux, real decode
- Real frame data, real packet data, real upload, real audio output

## 15. Diff Base Plan

**Base Commit**: a44fae9 (P2-REAL-001 FFmpeg proof)  
**Current Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof  
**Diff Scope**: Only E2E test and related pipeline files

## 16. Threading/COM/Device Ownership Plan

### 16.1 Thread Ownership
- E2E test must be single-threaded
- No cross-thread pipeline operations
- Each pipeline stage runs sequentially

### 16.2 COM/Device Rules
- D3D11 device context manages COM
- WASAPI writer manages COM
- E2E test does not directly manage COM

## 17. Failure Injection Plan

### 17.1 Required Failure Tests
1. File not found failure
2. FFmpeg probe failure
3. FFmpeg demux failure
4. FFmpeg decode failure
5. D3D11 upload failure (when available)
6. PCM conversion failure
7. WASAPI write failure (when available)

### 17.2 Failure Propagation
- Failures must propagate to E2E test
- No silent failure swallowing
- Clear error reporting

## 18. Resource Release Proof Plan

### 18.1 Required Release Tests
1. FFmpeg context release
2. D3D11 device/texture release
3. WASAPI writer release
4. Converter release
5. Resource leak detection

### 18.2 Release Verification
- Verify all resources released in test
- No dangling handles
- Clean shutdown proof

## 19. Schema Gate Plan

### 19.1 Required Evidence Fields
- `source_reading`: Real file reading success
- `probe_result`: Real FFmpeg probe result
- `demux_result`: Real FFmpeg demux result
- `decode_result`: Real FFmpeg decode result
- `video_upload`: Real D3D11 upload (or skipped)
- `audio_conversion`: Real PCM conversion
- `audio_output`: Real WASAPI output (or skipped)
- `frames_decoded`: Real count of decoded frames
- `frames_uploaded`: Real count of uploaded frames

### 19.2 Validation Rules
- `source_reading` must be true for RUNTIME_PASS
- `probe_result` must be true for RUNTIME_PASS
- `demux_result` must be true for RUNTIME_PASS
- `decode_result` must be true for RUNTIME_PASS
- `frames_decoded` must be > 0 for RUNTIME_PASS

## 20. Redline Gate Plan

### 20.1 Forbidden Terms
- "state machine only"
- "stub FFmpeg"
- "mock demux"
- "simulated decode"
- "interface contract only"

### 20.2 Allowlist Terms
- "real FFmpeg"
- "real demux"
- "real decode"
- "real frame data"
- "real upload"
- "real audio output"

## 21. Final Report Rules

### 21.1 Classification Options
- `RUNTIME_PASS`: Real E2E local playback runtime
- `BLOCKED_ENV`: Missing sample or FFmpeg
- `CONTRACT_PASS`: Interface contract proven but not runtime
- `FAIL`: Required proof missing or conflicting

### 21.2 Forbidden Claims
- Never claim real E2E without real FFmpeg
- Never claim runtime PASS if BLOCKED_ENV
- Never claim real decode without real frame data
- Never claim real upload without real texture

## 22. Immediate Next Steps

1. Create evidence artifact at `artifacts/p2/evidence/LRD-008.json`
2. Update `backend_task_checklist.md` with LRD-008 DONE
3. Update `current_state.md` with LRD-008 completion
4. Proceed to LRD-009 Micro Soak Runtime Audit

## 23. Self-Check Checklist

- [ ] Evidence file created with correct schema
- [ ] Checklist updated with DONE status
- [ ] Current state updated
- [ ] No forbidden files modified
- [ ] No scope leak introduced
- [ ] Test family plan defined
- [ ] Runtime environment risks documented
- [ ] Anti-fake proof strategy defined
- [ ] Machine-readable evidence plan defined
- [ ] CMake registration plan defined
- [ ] Scope leak scan plan defined
- [ ] Diff base plan defined
- [ ] Threading/COM/device ownership plan defined
- [ ] Failure injection plan defined
- [ ] Resource release proof plan defined
- [ ] Schema gate plan defined
- [ ] Redline gate plan defined
- [ ] Final report rules defined
- [ ] Immediate next steps defined
- [ ] Self-check completed