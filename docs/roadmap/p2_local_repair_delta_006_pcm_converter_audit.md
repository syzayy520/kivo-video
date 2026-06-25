# LRD-006 PCM Converter Reality Audit

## 1. Task Overview

**Task ID**: LRD-006  
**Task Family**: local_repair_delta/audio  
**Depends On**: LRD-005  
**Goal**: PCM buffer derives from real decoded audio frame data; no placeholder silence or generated dummy PCM as runtime proof.

## 2. Current Status Analysis

**Current Implementation**: `decoded_audio_frame_converter.hpp/cpp`  
**Current Status**: CONTRACT_PASS (stub implementation)  
**Evidence**: V10-011 downgraded to CONTRACT_PASS per V8-002 audit

**Interface Contract**:
- `convert(const DecodedFrame& frame, const PcmFormat& target_format)`: Convert decoded audio frame to PCM
- `is_ready()`: Check if converter is initialized and ready
- `last_error()`: Get last error message
- `release()`: Release all resources

**Current Test Coverage**: 8 tests in `decoded_audio_frame_converter_test.cpp`

## 3. Reality Proof Requirements

### 3.1 Real PCM Buffer Derivation
- **Real Audio Data**: PCM buffer must derive from actual decoded audio frame data
- **Real Sample Conversion**: Must convert actual samples, not generate placeholder silence
- **Real Format Handling**: Must handle actual sample formats (s16, s32, flt, dbl)
- **Real Channel Mapping**: Must handle actual channel configurations

### 3.2 No Placeholder/Dummy Proof
- **No Silence Generation**: Cannot generate silent PCM buffer as proof
- **No Dummy Data**: Cannot use pre-generated dummy PCM data
- **No Mock Conversion**: Must perform actual sample conversion
- **No Interface-Only Proof**: Must prove actual conversion, not just interface contract

### 3.3 Conversion Verification
- **Input/Output Verification**: Verify PCM data actually derives from input frame
- **Sample Accuracy**: Verify converted samples are accurate (not all zeros)
- **Format Correctness**: Verify output format matches target format
- **Timestamp Preservation**: Verify presentation timestamp preserved

## 4. Allowed/Forbidden Files

### 4.1 Allowed Production Files
- `backend/include/kivo/cinema_engine/audio_output/decoded_audio_frame_converter.hpp`
- `backend/src/audio_output/decoded_audio_frame_converter.cpp`
- `backend/include/kivo/cinema_engine/audio_output/wasapi_shared_pcm_writer.hpp`
- `backend/src/audio_output/wasapi_shared_pcm_writer.cpp`

### 4.2 Allowed Test Files
- `backend/tests/audio_output/decoded_audio_frame_converter_test.cpp`
- `backend/tests/audio_output/wasapi_shared_pcm_writer_test.cpp`

### 4.3 Forbidden Files
- No new families or directories
- No modifications to decode core, clock, seek, or other non-audio families
- No modifications to presenter or D3D11 families

## 5. Dependency Direction

```
DecodedAudioFrameConverter
  ↑
DecodedFrame (input)
  ↑
PcmFormat (target format)
```

**Direction**: Converter depends on DecodedFrame and PcmFormat. No reverse dependencies.

## 6. Single-File Responsibility Table

| File | Responsibility | Max Lines |
|------|---------------|-----------|
| decoded_audio_frame_converter.hpp | Interface definition | ≤100 |
| decoded_audio_frame_converter.cpp | Interface implementation | ≤150 |
| decoded_audio_frame_converter_test.cpp | Reality proof tests | ≤200 |

## 7. Test Family Plan

### 7.1 Required Reality Tests
1. **Real Audio Data Conversion Test**: Convert actual decoded audio frame with real samples
2. **Real Sample Accuracy Test**: Verify converted samples are accurate (not all zeros)
3. **Real Format Handling Test**: Handle actual sample formats (s16, s32, flt, dbl)
4. **Real Channel Mapping Test**: Handle actual channel configurations
5. **No Placeholder Proof Test**: Verify no silent PCM buffer generation
6. **Timestamp Preservation Test**: Verify presentation timestamp preserved

### 7.2 Test Requirements
- All tests must use real `DecodedFrame` with actual audio data
- No mock frames or stub conversions
- Tests must verify actual sample conversion (not just interface contract)
- Evidence must prove real PCM derivation, not just interface

## 8. Runtime Environment Risk

**Risk**: WASAPI may be unavailable on non-Windows systems  
**Mitigation**:
- Focus on PCM conversion (platform-neutral)
- WASAPI writer tests can be skipped on non-Windows
- Converter tests should work on all platforms
- Document platform requirements clearly

## 9. Sample Availability Risk

**Risk**: No sample audio with real decoded frames  
**Mitigation**:
- Generate real audio data in tests (sine wave)
- Use actual sample rates and channel configurations
- Verify conversion produces non-zero output
- Use existing 44100Hz stereo sample

## 10. STOP/BLOCKED/FAILED Conditions

### STOP Conditions
- Real decoded audio frame unavailable
- Real sample conversion fails
- No real audio data in frame
- Conversion produces all zeros

### BLOCKED Conditions
- No real audio sample available → `BLOCKED_ENV`
- Platform lacks required audio libraries → `BLOCKED_ENV`

### FAILED Conditions
- Interface contract only → `CONTRACT_PASS`
- Placeholder silence generation → `CONTRACT_PASS`
- Dummy PCM data → `CONTRACT_PASS`
- No real conversion proof → `CONTRACT_PASS`

## 11. Anti-Fake Proof Strategy

### 11.1 Real PCM Derivation Proof
- Must show actual `DecodedFrame` with real audio data
- Must show actual conversion producing non-zero output
- Must show sample accuracy (converted samples match input)
- Must show format correctness (output matches target format)

### 11.2 Forbidden Fake Proofs
- Silent PCM buffer (all zeros)
- Pre-generated dummy PCM data
- Mock conversion without actual processing
- Interface contract only

## 12. Machine-Readable Evidence Plan

### 12.1 Evidence File
- Path: `artifacts/p2/evidence/LRD-006.json`
- Schema: `p2-evidence-v8`
- Required fields: task_id, status, runtime_mode, runtime_environment, pcm_conversion, input_frame, output_pcm, conversion_result, sample_accuracy, format_correctness, inspector_snapshot, decision_ledger, errors, redaction, schema_validation, anti_fake, redline

### 12.2 Inspector Snapshot Requirements
- PCM conversion snapshot: input frame, output PCM, conversion result, sample accuracy, format correctness

### 12.3 Decision Ledger Requirements
- Conversion decision: input (frame, format), output (success, sample_count, bytes_per_sample)

## 13. CMake Registration Plan

### 13.1 Test Registration
- Add `kivo_pcm_converter_reality_test` to CMakeLists.txt
- Register under `backend/tests/audio_output/`
- Add to CTest with appropriate timeout

### 13.2 Build Verification
- Ensure audio_output family builds correctly
- Verify no forbidden includes or dependencies
- Confirm test links correctly

## 14. Scope Leak Scan Plan

### 14.1 Forbidden Terms
- Scan for: P3, remote, HDR, hardware decode, exclusive, passthrough, BDMV, ISO
- Scan for: placeholder silence, dummy PCM, mock conversion, interface contract only

### 14.2 Allowed Terms
- Local playback, PCM conversion, sample conversion, format handling
- Real audio data, actual samples, non-zero output, timestamp preservation

## 15. Diff Base Plan

**Base Commit**: a44fae9 (P2-REAL-001 FFmpeg proof)  
**Current Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof  
**Diff Scope**: Only audio_output converter files and tests

## 16. Threading/COM/Device Ownership Plan

### 16.1 Thread Ownership
- PCM conversion must be thread-safe
- No cross-thread conversion conflicts
- Converter can be used from any thread

### 16.2 COM/Device Rules
- PCM conversion is platform-neutral
- No COM dependencies in converter
- WASAPI writer has COM dependencies (separate)

## 17. Failure Injection Plan

### 17.1 Required Failure Tests
1. Empty frame conversion failure
2. Invalid format conversion failure
3. Unsupported channel configuration failure
4. Resource release failure

### 17.2 Failure Propagation
- Failures must propagate to converter
- No silent failure swallowing
- Clear error reporting

## 18. Resource Release Proof Plan

### 18.1 Required Release Tests
1. Converter release after conversion
2. Resource leak detection
3. Clean shutdown proof

### 18.2 Release Verification
- Verify all resources released in tests
- No dangling handles
- Clean shutdown proof

## 19. Schema Gate Plan

### 19.1 Required Evidence Fields
- `input_frame`: Real DecodedFrame with audio data
- `output_pcm`: Real PCM data (not all zeros)
- `conversion_result`: Real conversion success/failure
- `sample_accuracy`: Real sample accuracy verification
- `format_correctness`: Real format correctness verification

### 19.2 Validation Rules
- `output_pcm` must not be empty for RUNTIME_PASS
- `sample_accuracy` must be true for RUNTIME_PASS
- `format_correctness` must be true for RUNTIME_PASS
- No placeholder silence allowed for RUNTIME_PASS

## 20. Redline Gate Plan

### 20.1 Forbidden Terms
- "placeholder silence"
- "dummy PCM"
- "mock conversion"
- "interface contract only"
- "generated PCM data"

### 20.2 Allowlist Terms
- "real audio data"
- "actual samples"
- "non-zero output"
- "sample conversion"
- "format handling"

## 21. Final Report Rules

### 21.1 Classification Options
- `RUNTIME_PASS`: Real PCM conversion with real audio data
- `BLOCKED_ENV`: No real audio sample available
- `CONTRACT_PASS`: Interface contract proven but not runtime
- `FAIL`: Required proof missing or conflicting

### 21.2 Forbidden Claims
- Never claim real conversion without real audio data
- Never claim runtime PASS if BLOCKED_ENV
- Never claim sample accuracy without real verification
- Never claim format correctness without real verification

## 22. Immediate Next Steps

1. Create evidence artifact at `artifacts/p2/evidence/LRD-006.json`
2. Update `backend_task_checklist.md` with LRD-006 DONE
3. Update `current_state.md` with LRD-006 completion
4. Proceed to LRD-007 WASAPI Shared Runtime Audit

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