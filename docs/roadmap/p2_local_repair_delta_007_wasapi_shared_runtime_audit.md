# LRD-007 WASAPI Shared Runtime Audit

## 1. Task Overview

**Task ID**: LRD-007  
**Task Family**: local_repair_delta/audio  
**Depends On**: LRD-006  
**Goal**: Real COM initialization, default render endpoint, shared-mode format negotiation, `IAudioClient`, `IAudioRenderClient`, `GetBuffer`, `ReleaseBuffer`, `Start`, `Stop`.

## 2. Current Status Analysis

**Current Implementation**: `wasapi_shared_pcm_writer.hpp/cpp` and `wasapi_internal.hpp`  
**Current Status**: CONTRACT_PASS (stub implementation)  
**Evidence**: V10-011 downgraded to CONTRACT_PASS per V8-002 audit

**Interface Contract**:
- `initialize(const AudioEndpointRuntimeContract& endpoint)`: Initialize WASAPI shared mode writer
- `write(const uint8_t* pcm_data, int32_t byte_size)`: Write PCM data to WASAPI buffer
- `available_frames()`: Get current buffer status
- `start()`: Start playback
- `stop()`: Stop playback
- `release()`: Release all resources

**Internal State** (`WasapiWriterImpl`):
- `IMMDeviceEnumerator* enumerator`: Device enumerator
- `IMMDevice* device`: Audio device
- `IAudioClient* audio_client`: Audio client
- `IAudioRenderClient* render_client`: Render client
- `WAVEFORMATEX* mix_format`: Mix format
- `UINT32 buffer_frame_count`: Buffer frame count
- `bool com_initialized`: COM initialization state
- `bool is_playing`: Playing state

## 3. Reality Proof Requirements

### 3.1 Real COM Initialization
- **Real COM**: Must initialize COM with `CoInitializeEx`
- **Real MTA**: Must use multithreaded apartment (MTA)
- **Real Cleanup**: Must call `CoUninitialize` on release

### 3.2 Real Default Render Endpoint
- **Real Device Enumeration**: Must enumerate audio devices
- **Real Default Device**: Must get default render endpoint
- **Real Device Properties**: Must read device properties

### 3.3 Real Shared-Mode Format Negotiation
- **Real Mix Format**: Must get default mix format
- **Real Format Negotiation**: Must negotiate shared-mode format
- **Real Sample Rate/Channels**: Must match actual device capabilities

### 3.4 Real WASAPI Interface Calls
- **Real IAudioClient**: Must initialize audio client
- **Real IAudioRenderClient**: Must get render client
- **Real GetBuffer**: Must get buffer for writing
- **Real ReleaseBuffer**: Must release buffer after writing
- **Real Start**: Must start audio client
- **Real Stop**: Must stop audio client

## 4. Allowed/Forbidden Files

### 4.1 Allowed Production Files
- `backend/include/kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp`
- `backend/src/audio_output/wasapi_shared_pcm_writer.cpp`
- `backend/include/kivo/cinema_engine/audio_output/wasapi_internal.hpp`
- `backend/include/kivo/cinema_engine/audio_core/audio_endpoint_contract.hpp`

### 4.2 Allowed Test Files
- `backend/tests/audio_output/wasapi_shared_pcm_writer_test.cpp`

### 4.3 Forbidden Files
- No new families or directories
- No modifications to presenter, D3D11, or other non-audio families
- No modifications to PCM converter (separate task)

## 5. Dependency Direction

```
WasapiSharedPcmWriter
  ↑
WasapiWriterImpl (internal state)
  ↑
IAudioClient, IAudioRenderClient (WASAPI interfaces)
  ↑
IMMDevice, IMMDeviceEnumerator (device enumeration)
```

**Direction**: Writer depends on WASAPI interfaces, which depend on COM. No reverse dependencies.

## 6. Single-File Responsibility Table

| File | Responsibility | Max Lines |
|------|---------------|-----------|
| wasapi_shared_pcm_writer.hpp | Interface definition | ≤100 |
| wasapi_shared_pcm_writer.cpp | Interface implementation | ≤200 |
| wasapi_internal.hpp | Internal state | ≤50 |
| wasapi_shared_pcm_writer_test.cpp | Reality proof tests | ≤200 |

## 7. Test Family Plan

### 7.1 Required Reality Tests
1. **Real COM Initialization Test**: Verify COM initialized with MTA
2. **Real Device Enumeration Test**: Verify device enumeration works
3. **Real Default Endpoint Test**: Verify default render endpoint obtained
4. **Real Format Negotiation Test**: Verify shared-mode format negotiation
5. **Real Audio Client Test**: Verify IAudioClient initialization
6. **Real Render Client Test**: Verify IAudioRenderClient obtained
7. **Real Buffer Operations Test**: Verify GetBuffer/ReleaseBuffer work
8. **Real Start/Stop Test**: Verify Start/Stop operations work

### 7.2 Test Requirements
- All tests must use real WASAPI interfaces (not stubs)
- Tests must handle device unavailable scenario honestly
- Evidence must prove real WASAPI runtime, not just interface contract
- Platform: Windows only (WASAPI is Windows-specific)

## 8. Runtime Environment Risk

**Risk**: WASAPI may be unavailable on non-Windows systems or CI  
**Mitigation**:
- Detect WASAPI availability before tests
- Return `BLOCKED_ENV` when WASAPI unavailable
- Never claim runtime PASS without real WASAPI
- Document Windows requirement clearly

## 9. Sample Availability Risk

**Risk**: No audio device available on headless systems  
**Mitigation**:
- Use existing audio endpoint contract
- Test with default audio device
- Handle device unavailable gracefully
- Document audio device requirement

## 10. STOP/BLOCKED/FAILED Conditions

### STOP Conditions
- COM initialization fails
- Device enumeration fails
- No default audio device
- Format negotiation fails
- Audio client initialization fails
- Buffer operations fail

### BLOCKED Conditions
- Non-Windows platform → `BLOCKED_ENV`
- No audio device available → `BLOCKED_ENV`
- WASAPI service disabled → `BLOCKED_ENV`

### FAILED Conditions
- Interface contract only → `CONTRACT_PASS`
- Stub WASAPI implementation → `CONTRACT_PASS`
- No real COM initialization → `CONTRACT_PASS`
- No real buffer operations → `CONTRACT_PASS`

## 11. Anti-Fake Proof Strategy

### 11.1 Real WASAPI Runtime Proof
- Must show real COM initialization with MTA
- Must show real device enumeration and default endpoint
- Must show real format negotiation
- Must show real IAudioClient/IAudioRenderClient usage
- Must show real GetBuffer/ReleaseBuffer operations
- Must show real Start/Stop operations

### 11.2 Forbidden Fake Proofs
- Stub WASAPI implementation
- Mock COM initialization
- Simulated buffer operations
- Interface contract only

## 12. Machine-Readable Evidence Plan

### 12.1 Evidence File
- Path: `artifacts/p2/evidence/LRD-007.json`
- Schema: `p2-evidence-v8`
- Required fields: task_id, status, runtime_mode, runtime_environment, wasapi_runtime, com_initialization, device_enumeration, format_negotiation, audio_client, render_client, buffer_operations, start_stop, inspector_snapshot, decision_ledger, errors, redaction, schema_validation, anti_fake, redline

### 12.2 Inspector Snapshot Requirements
- WASAPI runtime snapshot: COM state, device info, format, audio client, render client, buffer stats

### 12.3 Decision Ledger Requirements
- WASAPI operation decision: input (endpoint, PCM data), output (success, samples_written)

## 13. CMake Registration Plan

### 13.1 Test Registration
- Add `kivo_wasapi_shared_runtime_test` to CMakeLists.txt
- Register under `backend/tests/audio_output/`
- Add to CTest with appropriate timeout
- Conditional on Windows platform

### 13.2 Build Verification
- Ensure audio_output family builds correctly
- Verify WASAPI includes link correctly
- Confirm test links with Windows libraries

## 14. Scope Leak Scan Plan

### 14.1 Forbidden Terms
- Scan for: P3, remote, HDR, hardware decode, exclusive, passthrough, BDMV, ISO
- Scan for: stub WASAPI, mock COM, simulated buffer, interface contract only

### 14.2 Allowed Terms
- Local playback, WASAPI shared, COM initialization, device enumeration
- Audio client, render client, buffer operations, start/stop

## 15. Diff Base Plan

**Base Commit**: a44fae9 (P2-REAL-001 FFmpeg proof)  
**Current Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof  
**Diff Scope**: Only WASAPI audio_output files and tests

## 16. Threading/COM/Device Ownership Plan

### 16.1 Thread Ownership
- WASAPI operations must be single-threaded
- COM must be initialized on thread that uses it
- No cross-thread buffer operations

### 16.2 COM/Device Rules
- COM initialized with MTA on writer thread
- Device owned by single thread
- Audio client used only from initialization thread

## 17. Failure Injection Plan

### 17.1 Required Failure Tests
1. COM initialization failure
2. Device enumeration failure
3. Format negotiation failure
4. Audio client initialization failure
5. Buffer acquisition failure
6. Start/Stop failure

### 17.2 Failure Propagation
- Failures must propagate to writer
- No silent failure swallowing
- Clear error reporting

## 18. Resource Release Proof Plan

### 18.1 Required Release Tests
1. Audio client release
2. Render client release
3. Device release
4. Enumerator release
5. COM uninitialization
6. Resource leak detection

### 18.2 Release Verification
- Verify all COM resources released
- No dangling pointers
- Clean COM shutdown proof

## 19. Schema Gate Plan

### 19.1 Required Evidence Fields
- `com_initialized`: Real COM initialization state
- `device_enumerated`: Real device enumeration result
- `format_negotiated`: Real format negotiation result
- `audio_client_initialized`: Real audio client initialization
- `buffer_operations`: Real GetBuffer/ReleaseBuffer calls
- `start_stop_operations`: Real Start/Stop calls

### 19.2 Validation Rules
- `com_initialized` must be true for RUNTIME_PASS
- `device_enumerated` must be true for RUNTIME_PASS
- `format_negotiated` must be true for RUNTIME_PASS
- `buffer_operations` must be real for RUNTIME_PASS

## 20. Redline Gate Plan

### 20.1 Forbidden Terms
- "stub WASAPI"
- "mock COM"
- "simulated buffer"
- "interface contract only"
- "fake audio client"

### 20.2 Allowlist Terms
- "real COM initialization"
- "actual device enumeration"
- "real format negotiation"
- "real buffer operations"
- "WASAPI shared mode"

## 21. Final Report Rules

### 21.1 Classification Options
- `RUNTIME_PASS`: Real WASAPI shared mode runtime
- `BLOCKED_ENV`: WASAPI unavailable, honest block
- `CONTRACT_PASS`: Interface contract proven but not runtime
- `FAIL`: Required proof missing or conflicting

### 21.2 Forbidden Claims
- Never claim real WASAPI without real COM
- Never claim runtime PASS if BLOCKED_ENV
- Never claim buffer operations without real operations
- Never claim Start/Stop without real operations

## 22. Immediate Next Steps

1. Create evidence artifact at `artifacts/p2/evidence/LRD-007.json`
2. Update `backend_task_checklist.md` with LRD-007 DONE
3. Update `current_state.md` with LRD-007 completion
4. Proceed to LRD-008 Local E2E Runtime Audit

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