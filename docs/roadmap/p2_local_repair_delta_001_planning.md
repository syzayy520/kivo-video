# P2-LOCAL-REPAIR-DELTA-001 Planning Draft

Created: 2026-06-26
Task Family: local repair delta
Depends On: V8-009 Final Gate Sequencing Gate
Status: PLANNING

---

## 1. Current Tree Inventory

### Production modules (backend/include/kivo/cinema_engine/)
- `source_core/` — 41 headers (local file, WebDAV, AList, Emby, network security)
- `demux_core/` — 12 headers (real_demux_runtime, real_probe_runtime, kivo_packet)
- `decode_core/` — 10 headers (real_software_decode_runtime, decoded_frame, decode_result)
- `video_render_core/` — 12 headers (d3d11_presenter, video_presenter_contract, fake_presenter_service)
- `audio_output/` — decoded_audio_frame_converter.hpp, wasapi_shared_pcm_writer.hpp
- `video_upload/` — decoded_video_frame_uploader.hpp, yuv_rgb_conversion.hpp, d3d11_device_context.hpp, d3d11_texture_upload.hpp
- `presenter/` — presenter_boundary_shell.hpp
- `media_timeline_core/` — 5 headers (master_clock, timebase_normalization, av_sync_policy)
- `playback_core/` — state machine, playback state
- `playback_inspector_core/` — inspector runtime
- `decision_ledger_core/` — decision ledger
- `graph_core/` — graph node/edge
- `third_party_adapter/ffmpeg_adapter/` — ffmpeg_adapter.hpp/cpp (adapter boundary)

### V10 Runtime Files (Existing)
- `real_demux_runtime.hpp/cpp` — FFmpeg demux with KivoPacket.data
- `real_software_decode_runtime.hpp/cpp` — FFmpeg decode with adopt_codec_context
- `real_probe_runtime.hpp/cpp` — FFmpeg probe
- `ffmpeg_adapter.hpp/cpp` — FFmpeg adapter boundary (opaque handles)
- `d3d11_presenter.hpp` — PresenterRuntimeContract
- `audio_endpoint_contract.hpp` — AudioEndpointRuntimeContract
- `master_clock.hpp` — MasterClock
- `decoded_frame.hpp` — DecodedFrame struct

---

## 2. Current V10 Evidence Audit Plan

### V8-002 Audit Results (22 items total)
- **RUNTIME_PASS**: 18 items (V10-007, V10-008, V10-012-V10-020, V10-022-V10-028)
- **CONTRACT_PASS**: 4 items (V10-009, V10-010, V10-011, V10-021)
- **FAIL**: 0 items
- **BLOCKED_ENV**: 0 items
- **NEEDS_REPAIR**: 0 items

### Downgrade Decisions
1. **V10-009**: D3D11 upload stub → CONTRACT_PASS (interface contract only)
2. **V10-010**: Presenter stub → CONTRACT_PASS (interface contract only)
3. **V10-011**: PCM stub → CONTRACT_PASS (interface contract only)
4. **V10-021**: SoakSimulator → CONTRACT_PASS (simulator not real runtime)

### Consistency Gaps Identified
- V10-027 report claims PASS_READY_FOR_REVIEW; audit finds 4 items are CONTRACT_PASS, not RUNTIME_PASS
- V10 Required Artifacts subsection still has TODO items
- V10 Required Tests subsection still has TODO items

### Repair Scope
- D3D11 device/context reality proof (LRD-003, LRD-004)
- Presenter boundary reality proof (LRD-005)
- PCM converter reality proof (LRD-006)
- WASAPI shared runtime proof (LRD-007)
- Local E2E runtime proof (LRD-008)
- Micro soak runtime proof (LRD-009)

---

## 3. Allowed Files

### Production (Local Repair New Code)
- `backend/include/kivo/cinema_engine/video_upload/` — D3D11 device/texture upload family
- `backend/include/kivo/cinema_engine/audio_output/` — PCM converter, WASAPI writer family
- `backend/include/kivo/cinema_engine/presenter/` — presenter boundary shell
- `backend/include/kivo/cinema_engine/playback_core/` — lifecycle controller
- `backend/include/kivo/cinema_engine/playback_inspector_core/` — inspector snapshot
- `backend/include/kivo/cinema_engine/decision_ledger_core/` — decision ledger entries

### Test (Local Repair New Tests)
- `backend/tests/video_upload/` — D3D11 reality audit tests
- `backend/tests/audio_output/` — PCM/WASAPI reality audit tests
- `backend/tests/integration_local/` — local E2E, micro soak, failure injection tests
- `backend/tests/evidence/` — evidence schema validation tests

### Modified (Existing Files)
- `backend/src/video_upload/` — D3D11 real implementation
- `backend/src/audio_output/` — PCM/WASAPI real implementation
- `backend/src/presenter/` — presenter real implementation
- `backend/CMakeLists.txt` — register new targets
- `CMakePresets.json` — add local repair presets

---

## 4. Forbidden Files

- Any P3 code (hardware decode, HDR output, exclusive audio, passthrough)
- Any remote code (WebDAV, AList, Emby, SMB, NFS, S3, IPTV)
- Any UI code (Qt, QML, media library)
- Any BDMV/ISO code
- Any new fake service code (local repair uses real runtime only)
- Any root-level temporary outputs (build_output.txt, tmp/)
- Any evidence files in source root (must be in artifacts/p2/evidence/)

---

## 5. Dependency Direction

```
video_upload → decode (DecodedFrame)
video_upload → third_party_adapter (D3D11 opaque handle)
audio_output → decode (DecodedFrame)
audio_output → third_party_adapter (WASAPI opaque handle)
presenter → video_upload
lifecycle → graph, presenter, audio_output, inspector
inspector ← all modules (read-only observer)
```

No circular dependencies. All D3D11/WASAPI types stay inside adapter boundary.

---

## 6. Single-File Responsibility Table

| File | Responsibility |
|------|---------------|
| d3d11_device_context.hpp | D3D11 device/context lifecycle (opaque handle) |
| d3d11_texture_upload.hpp | Texture creation + upload proof |
| decoded_video_frame_uploader.hpp | Interface: upload DecodedFrame to GPU texture |
| yuv_rgb_conversion.hpp | YUV→RGB/NV12 pixel format conversion policy |
| decoded_audio_frame_converter.hpp | DecodedFrame→PCM format conversion |
| wasapi_shared_pcm_writer.hpp | WASAPI shared mode PCM buffer write |
| presenter_boundary_shell.hpp | Presenter boundary: receive texture, record proof |
| playback_session_controller.hpp | Lifecycle: open→play→pause→resume→seek→stop→close |
| playback_inspector_snapshot.hpp | Inspector snapshot: all key decisions |
| decision_ledger_entry.hpp | Decision ledger: monotonic ordered decisions |

---

## 7. Test Family Plan

| Test | Type | Scope | Status |
|------|------|-------|--------|
| kivo_real_demux_decode_integration_test | Unit/Integration | P2-REAL-001 preservation | EXISTING (RUNTIME_PASS) |
| kivo_d3d11_first_frame_test | Integration | D3D11 texture upload proof | EXISTING (CONTRACT_PASS) |
| kivo_presenter_boundary_first_frame_test | Integration | Presenter boundary proof | EXISTING (CONTRACT_PASS) |
| kivo_decoded_audio_frame_converter_test | Integration | PCM conversion proof | EXISTING (CONTRACT_PASS) |
| kivo_wasapi_shared_pcm_writer_test | Integration | WASAPI shared PCM proof | EXISTING (CONTRACT_PASS) |
| kivo_clock_av_sync_test | Unit | Master clock + AV sync | EXISTING (RUNTIME_PASS) |
| kivo_seek_flush_generation_test | Unit | Seek/flush/generation | EXISTING (RUNTIME_PASS) |
| kivo_lifecycle_test | Unit | Lifecycle state machine | EXISTING (RUNTIME_PASS) |
| kivo_local_playback_e2e_test | Integration | Full end-to-end local playback | EXISTING (RUNTIME_PASS) |
| kivo_micro_soak_test | Integration | 30s bounded soak | EXISTING (CONTRACT_PASS) |
| kivo_failure_injection_test | Integration | Failure injection coverage | EXISTING (RUNTIME_PASS) |
| kivo_d3d11_reality_audit_test | Integration | Real D3D11 device/context | NEW (PLANNED) |
| kivo_presenter_reality_audit_test | Integration | Real presenter boundary | NEW (PLANNED) |
| kivo_pcm_reality_audit_test | Integration | Real PCM conversion | NEW (PLANNED) |
| kivo_wasapi_reality_audit_test | Integration | Real WASAPI shared | NEW (PLANNED) |
| kivo_local_e2e_reality_audit_test | Integration | Real local E2E runtime | NEW (PLANNED) |
| kivo_micro_soak_reality_audit_test | Integration | Real 30s soak | NEW (PLANNED) |

---

## 8. Runtime Environment Risk

| Risk | Mitigation | Status |
|------|-----------|--------|
| D3D11 device unavailable (CI/headless) | Use WARP adapter as fallback; test must detect and report BLOCKED_ENV | PLANNED |
| WASAPI device unavailable (CI/headless) | Use loopback or silence device; test must detect and report BLOCKED_ENV | PLANNED |
| FFmpeg ON build fails | BLOCKED condition; do not proceed | EXISTING (PASS) |
| No real sample available | BLOCKED condition; sample gate required | EXISTING (PASS) |
| COM apartment model issues | Use MTA for D3D11/WASAPI; document in threading plan | PLANNED |

---

## 9. Sample Availability Risk

| Sample | Status | Path | Notes |
|--------|--------|------|-------|
| H.264/AAC MP4 (320x240, 1s) | EXISTS | tests/real/test_sample.mp4 | For basic tests |
| H.264/AAC MP4 (30s+) for soak | NEEDED | Must create for LRD-009 | For micro soak |
| H.264/AC3 MKV (optional) | NOT YET | Optional for extended tests | For format coverage |

---

## 10. STOP / BLOCKED / FAILED Conditions

- **STOP_BRANCH_BASE_INVALID**: Branch or base commit mismatch
- **STOP_WORKSPACE_DIRTY**: Unexpected code changes in workspace
- **BLOCKED_SAMPLE_MISSING**: No real sample file available
- **BLOCKED_SAMPLE_TOO_SHORT**: Sample < 30s for micro soak
- **BLOCKED_D3D11_ENV**: D3D11 device unavailable in test environment
- **BLOCKED_WASAPI_ENV**: WASAPI endpoint unavailable in test environment
- **FAILED_SCOPE_LEAK**: P3/Remote/UI terms found in new code
- **FAILED_REAL_RUNTIME**: Fake runtime used instead of real
- **FAILED_ANTI_FAKE**: open decoder counted as decode, read packet counted as frame
- **FAILED_EVIDENCE_CONFLICT**: Report claims contradict evidence status

---

## 11. Anti-Fake Proof Strategy

Every local repair task must prove:
1. **Real input**: actual file bytes, actual compressed packets
2. **Real runtime**: actual D3D11 API call, actual WASAPI API call, actual PCM conversion
3. **Real output**: actual texture upload, actual PCM samples written
4. **Real assertion**: assert on output properties, not just success flag
5. **Inspector evidence**: Inspector snapshot records the real path
6. **Scope proof**: no forbidden terms in new code

### Forbidden Substitutions
- stub ≠ real D3D11 device
- placeholder PCM ≠ real audio conversion
- SoakSimulator ≠ real 30s soak
- interface contract ≠ runtime proof
- log-only proof ≠ real upload
- bool-only proof ≠ real texture

### V10 Contract Pass Items Must Become Runtime Pass
1. V10-009: D3D11 upload stub → real D3D11 upload (LRD-004)
2. V10-010: Presenter stub → real presenter boundary (LRD-005)
3. V10-011: PCM stub → real PCM conversion (LRD-006)
4. V10-021: SoakSimulator → real 30s soak (LRD-009)

---

## 12. Machine-Readable Evidence Plan

### Required Artifacts
1. `playback_proof.json` — end-to-end playback proof
2. `inspector_snapshot.json` — Inspector state at completion
3. `decision_ledger.json` — all decisions made during playback
4. `micro_soak_metrics.json` — queue bounds, memory, timing
5. `ffmpeg_off_antifake.json` — FFmpeg OFF anti-fake proof
6. `evidence_schema_validation.json` — schema validation result

### New Artifacts (Local Repair)
7. `d3d11_reality_audit.json` — D3D11 device/context proof
8. `presenter_reality_audit.json` — presenter boundary proof
9. `pcm_reality_audit.json` — PCM conversion proof
10. `wasapi_reality_audit.json` — WASAPI shared runtime proof
11. `local_e2e_reality_audit.json` — local E2E runtime proof
12. `micro_soak_reality_audit.json` — real 30s soak proof

### Artifact Path Plan
All local repair artifacts go to:
```
artifacts/p2/evidence/
  LRD-001.json (planning)
  LRD-002.json (audit)
  LRD-003.json (D3D11 device)
  LRD-004.json (D3D11 upload)
  LRD-005.json (presenter)
  LRD-006.json (PCM converter)
  LRD-007.json (WASAPI shared)
  LRD-008.json (local E2E)
  LRD-009.json (micro soak)
  LRD-010.json (evidence artifact)
  LRD-011.json (final report)
  LRD-012.json (regression)
  LRD-013.json (classification)
```

---

## 13. CMake Registration Plan

### New CMake Targets
- `kivo_d3d11_reality_audit_test` (executable, links kivo_cinema_engine)
- `kivo_presenter_reality_audit_test` (executable)
- `kivo_pcm_reality_audit_test` (executable)
- `kivo_wasapi_reality_audit_test` (executable)
- `kivo_local_e2e_reality_audit_test` (executable)
- `kivo_micro_soak_reality_audit_test` (executable)

### Local Repair CMake Preset
- `vs-debug-ffmpeg-local-repair` — MSVC x64 Debug, KIVO_ENABLE_FFMPEG=ON, local repair tests enabled

---

## 14. Scope Leak Scan Plan

### Before Commit
Run forbidden token scan on all new/modified files:
- Pattern: `P3|Remote|UI|HDR|hardware.decode|exclusive|passthrough|BDMV|ISO|SMB|NFS|Emby|Plex|Jellyfin|WebDAV|AList|S3|IPTV|AirPlay|Cast|PiP|Cinema|Trakt`
- Exception: `docs/roadmap/*` (documentation only)
- Exception: `backend_task_checklist.md` (checklist only)
- Exception: comments in documentation files
- Any match in new production code = FAILED_SCOPE_LEAK

### Runtime Scope Rules
- Local file playback terms allowed: FFmpeg, D3D11, WASAPI shared, PCM
- Remote/network terms forbidden in runtime code
- UI/rendering terms forbidden in backend code

---

## 15. Diff Base Plan

- Base commit: `79c2d38` (V8-001 through V8-009 completion)
- All local repair changes diff from this base
- Final diff must show only local repair allowed files
- No scope leak in diff

---

## 16. Threading / COM / Device Ownership Plan

| Resource | Thread | COM Model | Ownership |
|----------|--------|-----------|-----------|
| FFmpeg context | Demux/decode thread | None (no COM) | RealDemuxRuntime/RealSoftwareDecodeRuntime |
| D3D11 device | Main/render thread | MTA | d3d11_device_context |
| D3D11 texture | Main/render thread | MTA | d3d11_texture_upload |
| WASAPI client | Audio thread | MTA | wasapi_shared_pcm_writer |
| Master clock | Any (thread-safe) | None | master_clock |

### Threading Proof Requirements
- Real thread IDs in Inspector snapshot
- No cross-thread violations
- All resources released on close
- COM initialization policy documented

---

## 17. Failure Injection Plan

| Failure | Injection Point | Expected Behavior |
|---------|----------------|-------------------|
| D3D11 device lost | device_context returns error | Error mapped, presenter stops |
| WASAPI buffer full | write returns capacity error | Retry/backpressure |
| Clock drift > threshold | av_sync_policy detects | Frame drop/insert decision |
| Seek during playback | seek_request | Flush + generation increment |
| Close during decode | close() called | Graceful shutdown, no leak |
| Source read failure | source read returns error | Error mapped, playback stops |

---

## 18. Resource Release Proof Plan

### On Close/Stop
- FFmpeg: avcodec_free_context, avformat_close_input
- D3D11: Release() on texture, device, context
- WASAPI: Release() on audio client, render client
- Threads: join all spawned threads
- Queues: clear all queues

### Proof Requirements
- Assert all pointers null after close
- Assert no handles open
- Inspector snapshot shows resource release
- DecisionLedger records cleanup decisions

---

## 19. Schema Gate Plan

### Evidence Schema Requirements
1. All evidence files must use `p2-evidence-v8` schema
2. All evidence files must include:
   - task_id, family, status, runtime_mode, runtime_environment_kind
   - media/source/extension identities
   - Inspector snapshot
   - DecisionLedger entries
   - errors, redaction state
   - schema_validation result
   - anti_fake assertions
   - redline scan result

### Schema Validation Command
- Validate all evidence files against schema
- Fail on missing required fields
- Fail on unknown status values
- Fail on fake runtime status

---

## 20. Redline Gate Plan

### Redline Vocabulary
- fake/stub/simulate/simulator/dummy/mock-only
- todo-pass/always-pass/state-only-pass
- silent fallback/not implemented but pass
- P3 false-complete/HDR complete/Atmos complete

### Allowlist Rules
- Allow forbidden words only in:
  - Forbidden Scope documentation
  - Known Limitations documentation
  - Next Recommendation documentation
  - Explicit CONTRACT_PASS explanation

### Scan Targets
1. Source code and tests
2. Evidence JSON files
3. Inspector artifacts
4. DecisionLedger artifacts
5. Final report

---

## 21. Final Report Rules

### Classification Options
1. **RUNTIME_PASS**: All local runtime boundaries pass with real evidence
2. **CONTRACT_PASS**: Interface contracts proven but not runtime
3. **FAIL**: Required runtime proof missing or conflicting
4. **BLOCKED_ENV**: Environment unavailable (D3D11/WASAPI)

### Report Requirements
- Branch: kivo-video-p2-real-001-repair-ffmpeg-proof
- HEAD: commit hash after local repair
- Remote: synced
- Base: 79c2d38 (V8 completion)
- Planning proof: this document
- Runtime environment: MSVC, C++20, FFmpeg 7.1
- Changed files by natural family
- Real playback proof: CTest results
- Machine-readable evidence: artifact listing
- Verification: build/test/privacy/scope
- Scope: in/out items
- Known limitations
- Next recommendation

### Forbidden Claims
- Never claim P2 FOUNDATION COMPLETE
- Never claim ALL PROVIDERS RUNTIME VERIFIED
- Never claim P3/HDR/Atmos/remote completion
- Never claim full playback closure without all providers

---

## 22. Immediate Next Steps

1. Execute LRD-002: V10 Evidence Downgrade / Repair Audit
2. Execute LRD-003: D3D11 Device Reality Audit
3. Execute LRD-004: D3D11 Real Decoded Frame Upload
4. Execute LRD-005: Presenter Boundary Reality Audit
5. Execute LRD-006: PCM Converter Reality Audit
6. Execute LRD-007: WASAPI Shared Runtime Audit
7. Execute LRD-008: Local E2E Runtime Audit
8. Execute LRD-009: Micro Soak Runtime Audit
9. Execute LRD-010: Evidence Artifact Audit
10. Execute LRD-011: Final Report Consistency Audit
11. Execute LRD-012: Regression Protection
12. Execute LRD-013: Classification

---

## 23. Self-Check Checklist

- [ ] Branch is `kivo-video-p2-real-001-repair-ffmpeg-proof`
- [ ] Base is `79c2d38` (V8 completion)
- [ ] No code changes outside local repair allowed files
- [ ] No forbidden terms in new code
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
- [ ] No premature P2 FOUNDATION COMPLETE claim
- [ ] No premature ALL PROVIDERS RUNTIME VERIFIED claim

---

Classification: PLANNING_DRAFT_CREATED
Evidence Output: artifacts/p2/evidence/LRD-001.json
Next Task: LRD-002 V10 Evidence Downgrade / Repair Audit