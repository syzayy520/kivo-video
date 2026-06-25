# LRD-009 Micro Soak Runtime Audit

## 1. Task Overview

**Task ID**: LRD-009  
**Task Family**: local_repair_delta/integration  
**Depends On**: LRD-008  
**Goal**: 30s continuous local playback runtime over sample; no `SoakSimulator`, no repeated open/close fake soak, queue/memory high-water proof.

## 2. Current Status Analysis

**Current Implementation**: `micro_soak_test.cpp`  
**Current Status**: CONTRACT_PASS (downgraded per V8-002 audit)  
**Evidence**: V10-021 downgraded because SoakSimulator not real runtime soak

**Current Implementation Details**:
- Uses `SoakSimulator` with `QueueStats` and `MemoryTracker`
- Tracks peak queue size and memory usage
- Detects unbounded growth
- Runs for 30 seconds with real FFmpeg pipeline

**Problem**: SoakSimulator is not real runtime soak; it's a simulation.

## 3. Reality Proof Requirements

### 3.1 Real Continuous Playback
- **Real 30s Playback**: Must run real demux/decode pipeline for 30+ seconds
- **Real Sample File**: Must use actual H.264+AAC MP4 sample
- **Real FFmpeg Pipeline**: Must use real FFmpeg functions throughout

### 3.2 Real Queue/Memory Tracking
- **Real Queue High-Water**: Must track actual queue sizes during playback
- **Real Memory Usage**: Must track actual memory allocations
- **Real Unbounded Growth Detection**: Must detect actual memory leaks

### 3.3 No SoakSimulator
- **No Simulation**: Cannot use SoakSimulator class
- **No Fake Counters**: Must use real counters from actual pipeline
- **No Repeated Open/Close**: Must run continuous pipeline, not repeated open/close

### 3.4 Real Metrics Collection
- **Real Frame Counts**: Must count actual decoded frames
- **Real Byte Counts**: Must count actual bytes processed
- **Real Error Counts**: Must count actual errors
- **Real Duration**: Must measure actual elapsed time

## 4. Allowed/Forbidden Files

### 4.1 Allowed Production Files
- `backend/tests/presenter/micro_soak_test.cpp`
- `backend/include/kivo/cinema_engine/demux_core/real_demux_runtime.hpp`
- `backend/src/demux_core/real_demux_runtime.cpp`
- `backend/include/kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp`
- `backend/src/decode_core/real_software_decode_runtime.cpp`
- `backend/include/kivo/cinema_engine/demux_core/real_probe_runtime.hpp`
- `backend/src/demux_core/real_probe_runtime.cpp`
- `backend/include/kivo/cinema_engine/video_upload/d3d11_device_context.hpp`
- `backend/src/video_upload/d3d11_device_context.cpp`
- `backend/include/kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp`
- `backend/src/audio_output/decoded_audio_frame_converter.cpp`
- `backend/include/kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp`
- `backend/src/audio_output/wasapi_shared_pcm_writer.cpp`

### 4.2 Allowed Test Files
- `backend/tests/presenter/micro_soak_test.cpp`

### 4.3 Forbidden Files
- No SoakSimulator class
- No fake counters or simulators
- No repeated open/close patterns
- No state machine simulations

## 5. Dependency Direction

```
MicroSoakTest
  ↑
RealProbeRuntime (file probing)
  ↑
RealDemuxRuntime (continuous demux)
  ↑
RealSoftwareDecodeRuntime (continuous decode)
  ↑
D3d11DeviceContext + D3d11TextureUpload (video upload)
  ↑
DecodedAudioFrameConverter (audio conversion)
  ↑
WasapiSharedPcmWriter (audio output)
```

**Direction**: Soak test depends on all pipeline components for continuous operation. No reverse dependencies.

## 6. Single-File Responsibility Table

| File | Responsibility | Max Lines |
|------|---------------|-----------|
| micro_soak_test.cpp | 30s continuous playback proof | ≤400 |
| SoakMetrics | Real metrics collection | ≤50 |

## 7. Test Family Plan

### 7.1 Required Reality Tests
1. **30-Second Continuous Playback Test**: Run real pipeline for 30+ seconds
2. **Real Queue High-Water Test**: Track actual queue sizes during playback
3. **Real Memory Usage Test**: Track actual memory allocations
4. **Real Unbounded Growth Test**: Detect actual memory leaks
5. **Real Frame Count Test**: Count actual decoded frames
6. **Real Byte Count Test**: Count actual bytes processed
7. **Real Error Count Test**: Count actual errors
8. **Real Duration Test**: Measure actual elapsed time

### 7.2 Test Requirements
- All tests must use real FFmpeg functions (not stubs)
- Must run continuously for 30+ seconds
- Must track real metrics from actual pipeline
- Must handle device unavailable scenarios honestly

## 8. Runtime Environment Risk

**Risk**: 30s test may timeout on slow systems  
**Mitigation**:
- Set appropriate test timeout (60s)
- Use efficient sample file
- Skip device-specific tests when unavailable
- Document performance requirements

## 9. Sample Availability Risk

**Risk**: No sample video/audio file available  
**Mitigation**:
- Use existing 320x240 H.264 + 44100Hz AAC sample
- Verify sample exists before test
- Document sample requirements
- Handle missing sample gracefully

## 10. STOP/BLOCKED/FAILED Conditions

### STOP Conditions
- Sample file not found
- FFmpeg not available
- Real pipeline fails to start
- Real pipeline crashes during soak
- Memory leak detected

### BLOCKED Conditions
- No sample file → `BLOCKED_ENV`
- FFmpeg not compiled → `BLOCKED_ENV`
- D3D11 not available → video upload skipped (not blocked)
- WASAPI not available → audio output skipped (not blocked)

### FAILED Conditions
- SoakSimulator used → `CONTRACT_PASS`
- Fake counters used → `CONTRACT_PASS`
- Repeated open/close → `CONTRACT_PASS`
- No real metrics → `CONTRACT_PASS`

## 11. Anti-Fake Proof Strategy

### 11.1 Real Soak Runtime Proof
- Must show real 30+ second continuous playback
- Must show real queue high-water marks
- Must show real memory usage tracking
- Must show real frame/byte counts
- Must show real error counts
- Must show real duration measurement

### 11.2 Forbidden Fake Proofs
- SoakSimulator class usage
- Fake counters or simulators
- Repeated open/close patterns
- State machine simulations
- Interface contract only

## 12. Machine-Readable Evidence Plan

### 12.1 Evidence File
- Path: `artifacts/p2/evidence/LRD-009.json`
- Schema: `p2-evidence-v8`
- Required fields: task_id, status, runtime_mode, runtime_environment, soak_test, duration, queue_high_water, memory_usage, frame_counts, byte_counts, error_counts, d3d11_status, wasapi_status, inspector_snapshot, decision_ledger, errors, redaction, schema_validation, anti_fake, redline

### 12.2 Inspector Snapshot Requirements
- Soak test snapshot: duration, queue high water, memory usage, frame counts, byte counts, error counts

### 12.3 Decision Ledger Requirements
- Soak decision: input (sample_path), output (success, duration, frames_decoded, bytes_processed)

## 13. CMake Registration Plan

### 13.1 Test Registration
- Add `kivo_micro_soak_runtime_test` to CMakeLists.txt
- Register under `backend/tests/presenter/`
- Add to CTest with appropriate timeout (60s)
- Conditional on FFmpeg availability

### 13.2 Build Verification
- Ensure soak test builds correctly
- Verify FFmpeg links correctly
- Confirm test runs within timeout

## 14. Scope Leak Scan Plan

### 14.1 Forbidden Terms
- Scan for: P3, remote, HDR, hardware decode, exclusive, passthrough, BDMV, ISO
- Scan for: SoakSimulator, fake counters, repeated open/close, state machine

### 14.2 Allowed Terms
- Real soak, continuous playback, real metrics, queue high water, memory usage
- Real frame counts, real byte counts, real error counts

## 15. Diff Base Plan

**Base Commit**: a44fae9 (P2-REAL-001 FFmpeg proof)  
**Current Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof  
**Diff Scope**: Only soak test and related pipeline files

## 16. Threading/COM/Device Ownership Plan

### 16.1 Thread Ownership
- Soak test must be single-threaded
- No cross-thread pipeline operations
- Each pipeline stage runs sequentially

### 16.2 COM/Device Rules
- D3D11 device context manages COM
- WASAPI writer manages COM
- Soak test does not directly manage COM

## 17. Failure Injection Plan

### 17.1 Required Failure Tests
1. Memory leak detection test
2. Queue overflow detection test
3. Error rate monitoring test
4. Duration accuracy test

### 17.2 Failure Propagation
- Failures must propagate to soak test
- No silent failure swallowing
- Clear error reporting

## 18. Resource Release Proof Plan

### 18.1 Required Release Tests
1. FFmpeg context release after soak
2. D3D11 device/texture release after soak
3. WASAPI writer release after soak
4. Converter release after soak
5. Resource leak detection

### 18.2 Release Verification
- Verify all resources released after soak
- No dangling handles
- Clean shutdown proof

## 19. Schema Gate Plan

### 19.1 Required Evidence Fields
- `duration`: Real elapsed time (≥30 seconds)
- `queue_high_water`: Real queue high water mark
- `memory_usage`: Real memory usage tracking
- `video_frames_decoded`: Real count of decoded video frames
- `audio_frames_decoded`: Real count of decoded audio frames
- `video_bytes_processed`: Real count of video bytes processed
- `audio_bytes_processed`: Real count of audio bytes processed
- `error_counts`: Real error counts

### 19.2 Validation Rules
- `duration` must be ≥30 seconds for RUNTIME_PASS
- `video_frames_decoded` must be > 0 for RUNTIME_PASS
- `audio_frames_decoded` must be > 0 for RUNTIME_PASS
- No SoakSimulator allowed for RUNTIME_PASS

## 20. Redline Gate Plan

### 20.1 Forbidden Terms
- "SoakSimulator"
- "fake counters"
- "repeated open/close"
- "state machine simulation"
- "interface contract only"

### 20.2 Allowlist Terms
- "real soak"
- "continuous playback"
- "real metrics"
- "queue high water"
- "memory usage"
- "frame counts"

## 21. Final Report Rules

### 21.1 Classification Options
- `RUNTIME_PASS`: Real 30s continuous playback with real metrics
- `BLOCKED_ENV`: Missing sample or FFmpeg
- `CONTRACT_PASS`: Interface contract proven but not runtime
- `FAIL`: Required proof missing or conflicting

### 21.2 Forbidden Claims
- Never claim real soak without real 30s playback
- Never claim runtime PASS if BLOCKED_ENV
- Never claim real metrics without real counters
- Never claim queue high water without real tracking

## 22. Immediate Next Steps

1. Create evidence artifact at `artifacts/p2/evidence/LRD-009.json`
2. Update `backend_task_checklist.md` with LRD-009 DONE
3. Update `current_state.md` with LRD-009 completion
4. Proceed to LRD-010 Evidence Artifact Audit

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