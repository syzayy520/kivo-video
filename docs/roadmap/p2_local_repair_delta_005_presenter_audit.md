# LRD-005 Presenter Boundary Reality Audit

## 1. Task Overview

**Task ID**: LRD-005  
**Task Family**: local_repair_delta/video  
**Depends On**: LRD-004  
**Goal**: Presenter receives real uploaded texture boundary proof; unavailable device does not become runtime PASS.

## 2. Current Status Analysis

**Current Implementation**: `presenter_boundary_shell.hpp/cpp`  
**Current Status**: CONTRACT_PASS (stub implementation)  
**Evidence**: V10-010 downgraded to CONTRACT_PASS per V8-002 audit

**Interface Contract**:
- `initialize()`: Initialize presenter boundary
- `present(D3D11TextureHandle texture, int32_t width, int32_t height)`: Receive uploaded texture
- `first_frame_time_us()`: Get first frame presentation time
- `frames_presented()`: Get total frames presented
- `has_first_frame()`: Check if first frame presented
- `status()`: Get presenter status
- `release()`: Release all resources
- `is_ready()`: Check if ready

**Current Test Coverage**: 7 tests in `presenter_boundary_first_frame_test.cpp`

## 3. Reality Proof Requirements

### 3.1 Real Texture Boundary Proof
- **Real Texture Handle**: Presenter must receive actual `D3D11TextureHandle` from real upload
- **Real Dimensions**: Width/height must match actual texture dimensions
- **Real Presentation**: Presenter must actually present texture (not just record timestamp)
- **Real First Frame Detection**: First frame must be from actual texture presentation

### 3.2 Unavailable Device Handling
- **Device Unavailable**: When D3D11 device not available, presenter must not claim runtime PASS
- **Honest Block**: Unavailable device must result in `BLOCKED_ENV` status
- **No Fake Presentation**: Cannot simulate presentation without real device

### 3.3 Presentation Verification
- **Texture Content**: Presenter should verify texture contains actual data
- **Presentation Timestamp**: First frame timestamp must be from real presentation
- **Frame Counter**: Must increment only on real presentations

## 4. Allowed/Forbidden Files

### 4.1 Allowed Production Files
- `backend/include/kivo/cinema_engine/presenter/presenter_boundary_shell.hpp`
- `backend/src/presenter/presenter_boundary_shell.cpp`
- `backend/include/kivo/cinema_engine/video_render_core/video_presenter_contract.hpp`
- `backend/src/video_render_core/fake_presenter_service.cpp`

### 4.2 Allowed Test Files
- `backend/tests/presenter/presenter_boundary_first_frame_test.cpp`
- `backend/tests/video_render_core/presenter_boundary_test.cpp`

### 4.3 Forbidden Files
- No new families or directories
- No modifications to D3D11 device context or texture upload families
- No modifications to audio, clock, seek, or other non-presenter families

## 5. Dependency Direction

```
PresenterBoundaryShell
  ↑
D3d11TextureUpload (texture creation)
  ↑
D3d11DeviceContext (device initialization)
```

**Direction**: Presenter depends on texture upload, which depends on device context. No reverse dependencies.

## 6. Single-File Responsibility Table

| File | Responsibility | Max Lines |
|------|---------------|-----------|
| presenter_boundary_shell.hpp | Interface definition | ≤100 |
| presenter_boundary_shell.cpp | Interface implementation | ≤150 |
| presenter_boundary_first_frame_test.cpp | Reality proof tests | ≤200 |

## 7. Test Family Plan

### 7.1 Required Reality Tests
1. **Real Texture Presentation Test**: Present actual texture from real upload
2. **Real First Frame Detection Test**: Verify first frame from real presentation
3. **Real Frame Counter Test**: Verify frame counter increments on real presentations
4. **Real Timestamp Test**: Verify timestamps from real system clock
5. **Device Unavailable Block Test**: Verify BLOCKED_ENV when device unavailable
6. **No Fake Presentation Test**: Verify no simulation without real device

### 7.2 Test Requirements
- All tests must use real `D3d11DeviceContext` and `D3d11TextureUpload`
- No mock textures or stub presentations
- Tests must handle device unavailable scenario honestly
- Evidence must prove real texture boundary, not just interface contract

## 8. Runtime Environment Risk

**Risk**: D3D11 device may be unavailable on CI/headless systems  
**Mitigation**: 
- Detect device availability before tests
- Return `BLOCKED_ENV` when device unavailable
- Never claim runtime PASS without real device
- Document device requirements clearly

## 9. Sample Availability Risk

**Risk**: No sample video with real decoded frames  
**Mitigation**:
- Use existing 320x240 H.264 sample
- Decode real frames using existing FFmpeg pipeline
- Use decoded frame data for texture upload
- Verify texture contains actual pixel data

## 10. STOP/BLOCKED/FAILED Conditions

### STOP Conditions
- Real D3D11 device context unavailable
- Real texture creation fails
- Real texture upload fails
- Real presentation fails
- No real decoded frame data available

### BLOCKED Conditions
- D3D11 environment not available → `BLOCKED_ENV`
- No hardware or WARP device → `BLOCKED_ENV`
- Missing sample video → `BLOCKED_ENV`

### FAILED Conditions
- Interface contract only → `CONTRACT_PASS`
- Stub presentation → `CONTRACT_PASS`
- No real texture boundary proof → `CONTRACT_PASS`
- Device unavailable but claims runtime PASS → `FAIL`

## 11. Anti-Fake Proof Strategy

### 11.1 Real Texture Boundary Proof
- Must show actual `D3D11TextureHandle` passed to `present()`
- Must show actual width/height matching texture dimensions
- Must show actual first frame timestamp from real presentation
- Must show actual frame counter increment

### 11.2 Forbidden Fake Proofs
- Empty texture presentation
- Stub texture without real data
- Timestamp without real presentation
- Frame counter without real presentations
- Device unavailable but claims success

## 12. Machine-Readable Evidence Plan

### 12.1 Evidence File
- Path: `artifacts/p2/evidence/LRD-005.json`
- Schema: `p2-evidence-v8`
- Required fields: task_id, status, runtime_mode, runtime_environment, presenter_boundary, texture_handle, presentation_result, device_availability, inspector_snapshot, decision_ledger, errors, redaction, schema_validation, anti_fake, redline

### 12.2 Inspector Snapshot Requirements
- Presenter boundary snapshot: initialization, texture handle, presentation result, first frame time, frame count, device availability

### 12.3 Decision Ledger Requirements
- Presentation decision: input (texture, dimensions), output (success, first frame time, frame count)

## 13. CMake Registration Plan

### 13.1 Test Registration
- Add `kivo_presenter_boundary_reality_test` to CMakeLists.txt
- Register under `backend/tests/presenter/`
- Add to CTest with appropriate timeout

### 13.2 Build Verification
- Ensure presenter family builds correctly
- Verify no forbidden includes or dependencies
- Confirm test links correctly

## 14. Scope Leak Scan Plan

### 14.1 Forbidden Terms
- Scan for: P3, remote, HDR, hardware decode, exclusive, passthrough, BDMV, ISO
- Scan for: presenter simulation, stub presentation, fake texture

### 14.2 Allowed Terms
- Local playback, D3D11 hardware, WASAPI shared, FFmpeg, presenter boundary
- Real texture, real presentation, real first frame, device unavailable

## 15. Diff Base Plan

**Base Commit**: a44fae9 (P2-REAL-001 FFmpeg proof)  
**Current Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof  
**Diff Scope**: Only presenter boundary files and tests

## 16. Threading/COM/Device Ownership Plan

### 16.1 Thread Ownership
- Presenter operations must be single-threaded
- No cross-thread texture presentation
- Device context must be owned by single thread

### 16.2 COM/Device Rules
- D3D11 device context manages COM initialization
- Presenter does not directly manage COM
- Device ownership follows D3D11 device context rules

## 17. Failure Injection Plan

### 17.1 Required Failure Tests
1. Device initialization failure
2. Texture creation failure
3. Texture upload failure
4. Presentation failure
5. Resource release failure

### 17.2 Failure Propagation
- Failures must propagate to presenter boundary
- No silent failure swallowing
- Clear error reporting

## 18. Resource Release Proof Plan

### 18.1 Required Release Tests
1. Texture release after presentation
2. Device context release
3. Presenter release
4. Resource leak detection

### 18.2 Release Verification
- Verify all resources released in tests
- No dangling handles
- Clean shutdown proof

## 19. Schema Gate Plan

### 19.1 Required Evidence Fields
- `texture_handle`: Real D3D11 texture handle (not null)
- `presentation_result`: Real presentation success/failure
- `first_frame_time`: Real timestamp from presentation
- `frame_count`: Real count of presentations
- `device_availability`: Real device availability status

### 19.2 Validation Rules
- `texture_handle` must not be null for RUNTIME_PASS
- `presentation_result` must be true for RUNTIME_PASS
- `first_frame_time` must be > 0 for RUNTIME_PASS
- `device_availability` must be "available" for RUNTIME_PASS

## 20. Redline Gate Plan

### 20.1 Forbidden Terms
- "stub presentation"
- "simulated texture"
- "fake presenter"
- "interface contract only"
- "device unavailable but pass"

### 20.2 Allowlist Terms
- "real texture"
- "actual presentation"
- "real first frame"
- "device unavailable"
- "BLOCKED_ENV"

## 21. Final Report Rules

### 21.1 Classification Options
- `RUNTIME_PASS`: Real texture presented with real device
- `BLOCKED_ENV`: Device unavailable, honest block
- `CONTRACT_PASS`: Interface contract proven but not runtime
- `FAIL`: Required proof missing or conflicting

### 21.2 Forbidden Claims
- Never claim real presentation without real device
- Never claim runtime PASS if BLOCKED_ENV
- Never claim presentation verification without real verification
- Never claim first frame without real presentation

## 22. Immediate Next Steps

1. Create evidence artifact at `artifacts/p2/evidence/LRD-005.json`
2. Update `backend_task_checklist.md` with LRD-005 DONE
3. Update `current_state.md` with LRD-005 completion
4. Proceed to LRD-006 PCM Converter Reality Audit

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