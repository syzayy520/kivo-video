# Kivo Video Project Long-term Memory

## Project Overview
- **Repository**: https://github.com/syzayy520/kivo-video
- **Primary Branch**: kivo-video-p2-remote-range-runtime-http-webdav-001
- **Current Focus**: HTTP Range + WebDAV Bridge runtime complete (46 provider tests PASS)
- **Next Task**: AList/Emby/CloudDrive/HLS provider runtimes

## Key Architecture Decisions
1. **Iron Gate Rule**: conception → planning design → natural family tree → implementation → single file, single responsibility → no flat files
2. **Family-Based Organization**: Backend code organized by responsibility families, not technical layers
3. **No Dumping Grounds**: Prohibition of common, misc, or utility dumping-ground families
4. **Evidence Schema**: All evidence must use V8 status vocabulary (RUNTIME_PASS, CONTRACT_PASS, FAIL, BLOCKED_ENV, NOT_IMPLEMENTED, SKIPPED_TEST_ONLY)

## Protected Work (Do Not Repeat)
- FFmpeg probe/demux/decode skeleton
- Presenter abort repair (D3d11DeviceContext fix)
- D3D11 unavailable skip semantics (SKIP_REGULAR_EXPRESSION)
- wasapi_internal.hpp (WASAPI COM Pimpl struct)
- d3d11_internal.hpp (D3D11 Pimpl structs)
- FfmpegFrameResult frame data/linesize/plane count

## P2 Remaining Families Mapped
1. **local_repair_delta** (LRD-001 through LRD-013): Local playback proof, device reality audit, presenter boundary, PCM conversion, WASAPI shared mode, E2E runtime, micro soak, evidence artifacts, final report consistency, regression protection
2. **evidence_schema** (SGF-001 through SGF-008): Evidence schema validation, artifact path enforcement, schema versioning, privacy compliance, anti-fake validation
3. **redline_gate** (RGF-001 through RGF-005): Redline gate enforcement, forbidden claim scanning, report/source conflict detection, premature claim prevention
4. **format_compat** (FMT-001): Format compatibility matrix, codec support, container format validation, transcoding policy
5. **media_identity** (MID-001): Media identity resolution, fingerprinting, metadata extraction, content identification
6. **subtitle_chapter** (SUB-001): Subtitle rendering, chapter navigation, subtitle format support, chapter metadata
7. **source_core** (SRC-001): Source playable object contracts, source identity, source location, source access
8. **provider_runtime** (PMX-001, PMX-002): Provider runtime matrix generation, provider honesty gate, provider status tracking
9. **playback_state** (PST-001): Playback state management, state transitions, state persistence, state recovery
10. **audio_codec_policy** (ACP-001): Audio codec policy, codec selection, bitrate policy, channel mapping
11. **presentation_extension** (CAST-001, PIP-001, CIN-001, TRK-001): Cast foundation, PiP foundation, cinema foundation, Trakt foundation
12. **final_gate** (PFFG-001, PFFG-002, APRV-001): P2 foundation final gate, foundation final report, all providers runtime verification gate

## Provider Families
- **NET-001**: Network File Sources (SMB, NFS, FTP, FTPS, SFTP, HTTP/HTTPS)
- **OBJ-001**: Object Storage (S3, MinIO)
- **WDA-001**: WebDAV and AList
- **MSV-001**: Media Server (Emby, Plex, Jellyfin)
- **CLD-001**: Cloud Drive (Google Drive, OneDrive)
- **IPTV-001**: IPTV Live

## Build Environment
- **Visual Studio**: 18 (2026) at C:\Program Files\Microsoft Visual Studio\18\Community
- **vcvarsall.bat**: "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
- **Build Presets**: vs-debug, ninja-debug, vs-debug-ffmpeg
- **Test Command**: ctest --preset vs-debug --output-on-failure --timeout 30

## Last Verification
- **Branch**: kivo-video-p2-remote-range-runtime-http-webdav-001
- **Status**: HTTP Range + WebDAV Bridge runtime COMPLETE
- **CTest (provider)**: 46/46 PASS (12 local_file + 25 HTTP range + 9 WebDAV bridge)
- **CTest (source core)**: 16/16 PASS
- **No Debug Dialog Gate**: PASS (all 4 new tests run without abort())
- **2 crash bugs fixed**: webdav/auth_test.cpp (nullptr deref) + webdav/collection_test.cpp (nullptr std::function call)

## V8 Execution Sequence Completion
- **V8-001**: Branch / Workspace / Root Clean Gate - DONE
- **V8-002**: Current Checklist Consistency Gate - DONE
- **V8-003**: No Duplicate Work Gate - DONE
- **V8-004**: Natural Family Tree Gate - DONE
- **V8-005**: Allowed / Forbidden Claims Gate - DONE
- **V8-006**: Status Vocabulary Gate - DONE
- **V8-007**: Evidence Path Gate - DONE
- **V8-008**: Provider Runtime Matrix Gate Plan - DONE
- **V8-009**: Final Gate Sequencing Gate - DONE
- **LRD-001 through LRD-013**: Local Repair Delta - ALL DONE
- **SGF-001 through SGF-008**: Schema Gate Foundation - ALL DONE
- **RGF-001 through RGF-005**: Redline Gate Foundation - ALL DONE
- **Provider Foundations**: 12 provider families - ALL DONE
- **Presentation Extensions**: 4 presentation extensions - ALL DONE
- **Provider Matrix**: PMX-001, PMX-002 - ALL DONE
- **Final Gates**: PFFG-001, PFFG-002, APRV-001 - ALL DONE

## V8 Final Status: TWO SEPARATE CLAIMS

### P2 Foundation (Planning & Documentation) — COMPLETE
- **Status**: P2 FOUNDATION STATUS (planning, docs, contracts, evidence schemas, gate definitions)
- **V8-001 through V8-009**: Global Gates — ALL DONE
- **LRD-001 through LRD-013**: Local Repair Delta (audit/planning) — ALL DONE
- **SGF-001 through SGF-008**: Schema Gate Foundation — ALL DONE
- **RGF-001 through RGF-005**: Redline Gate Foundation — ALL DONE
- **Provider Foundations**: 12 provider families with docs/contract evidence — ALL DONE
- **Presentation Extensions**: CAST-001, PIP-001, CIN-001, TRK-001 — ALL DONE
- **Provider Matrix**: PMX-001, PMX-002 — ALL DONE
- **Final Gates**: PFFG-001, PFFG-002, APRV-001 — ALL DONE (gate definitions)
- **Evidence**: Foundation evidence files created in artifacts/p2/evidence/ (static planning evidence, not runtime proof)

### P2 Local Runtime Verification — SEPARATE from Foundation
- **Status**: Requires real hardware (D3D11 GPU + WASAPI audio device) with real sample file
- **local_file (E2E)**: BLOCKED_ENV without all 4 pre-conditions (sample + FFmpeg + D3D11 + WASAPI)
- **local_file (Soak)**: BLOCKED_ENV without all 4 pre-conditions
- **Exit codes**: 0=RUNTIME_PASS, 1=BLOCKED_ENV, 2=FAIL — strictly enforced
- **RUNTIME_PASS requires**: ALL 4 pre-conditions met + frames_uploaded > 0 + frames_written > 0 + 30s continuous WASAPI write

### APRV-001 verification — POST-FOUNDATION ONLY
- **Status**: NOT YET (0 of 22 providers have RUNTIME_PASS)
- **This is explicitly NOT part of P2 Foundation completion**
- **Requires**: Every provider with real runtime proof, not CONTRACT_PASS or static JSON
- **Claim**: "APRV-001 final verification" is forbidden until every provider matrix entry is RUNTIME_PASS

## Permanent Execution Discipline Rules (User-Mandated, 2026-06-27)

These rules override any convenience defaults. Apply to ALL Kivo Video backend tasks.

1. **Executor not architect**: Only execute per ticket. Do not free-architect, do not fabricate, do not skip gates.
2. **Evidence only**: Every conclusion must come from real command output / git diff / test result / evidence. Without evidence, write only NOT_VERIFIED / BLOCKED / FAIL / SCHEMA_DEGRADED.
3. **Local project safety first**: Default to isolated git worktree. Forbidden: reset, clean, force push, deleting source. Original worktree is read-only.
4. **No premature PASS**: Never write PASS before a gate is actually run. Any gate failure = STOP immediately.
5. **CTest honesty**: Record Total/Passed/Failed. Direct Exe cannot replace CTest. Each Direct Exe must record spec name / actual exe name / path / timeout / exit code / logs / name mapping.
6. **Evidence/Card consistency**: evidence JSON and execution card MUST match (task id / commit model / repair count / total commits / schema gate / redline / WebDAV not_verified).
7. **Commit count anti-trap**: Before committing, compute current_count + 1 = expected_count. Never chase your own SHA.
8. **Schema Gate truth**: If validator unsupported, write SCHEMA_DEGRADED + VALIDATOR_UNSUPPORTED. Never write PASS.
9. **Redline full scan**: Any hit must list path + classification. Only production pollution = FAIL.
10. **Secret Gate honesty**: Tell the truth. Windows paths only write non-secret worktree paths.
11. **WebDAV 4 items permanent not_verified**: href exact-match, collection rejection, malformed XML fail-closed, large XML body cap — stay not_verified unless dedicated verification evidence exists.
12. **Failure = FAIL report**: On failure output FAIL report. Never continue writing "P2 closed" / "Ready for P3". Prefer STOP over fake PASS. Prefer failure report over fake closure.

## P2 Source Runtime Closure Status (2026-06-27)

- Trusted baseline HEAD: `c90cfdc938cd21e6ecf6d62d1b088d0d0fe3b9a1`
- closure-001: REJECTED (premature PASS on Direct Exe/no_debug_dialog not closed)
- closure-002: REJECTED (commit count self-reference infinite loop + evidence/card mismatch)
- closure-003: PASSED at HEAD `9ac330a` (locked, do not modify) — Model A single commit, no self-referential hash
- Current branch HEAD is back at `c90cfdc` (closure-003's 9ac330a is NOT on this branch's history line)
- P2 closure is COMPLETE. Next phase is P3 only.

## P3 DOCX Decomposition Strategy (2026-06-27)

- P3 source DOCX: `C:/Users/Administrator/Downloads/Kivo Video P3 完整线路设计规划 v9 Closure-Ready Ultimate.docx`
- User is having Codex decompose this DOCX into a numbered task checklist with gates.
- I (CodeBuddy) will execute the checklist when ready.
- Checklist requirements: each task needs unique ID, dependency chain, acceptance gate (name+command+pass condition), file allowlist, commit model, explicit not_verified boundaries.

## P3 Final Baseline Lock (2026-06-27, User-Accepted)

**P3 closed with accepted deferrals. P3 source/input bridge baseline locked. P4 requires explicit user authorization.**

- Final branch: `kivo-video-p3-preflight-gate-repair-001`
- Final HEAD: `791805e1378bbe675608c67cb01c40d9dd051ef9`
- Closure evidence: `artifacts/p3/evidence/FC-095.json`
- Closure report: `docs/roadmap/p3_complete_line_v9_closure_report.md`
- Tests: 106/106 PASS
- Architecture guard: 35 files / 0 violations / PASS
- Forbidden token: 1 comment_sentinel / 0 production pollution / PASS
- Accepted deferrals: FC-083 soak, FC-084 performance → P4
- P2 WebDAV 4 not_verified items preserved

**Forbidden rewrites:**
- NOT P3 complete no-deferral
- NOT P3 runtime performance PASS
- NOT P3 soak PASS
- NOT P4 started
- NOT Ready for P4 without user authorization

**P3 is locked. No more P3 modifications. P4 requires explicit user authorization.**

## P5 Video Media Plane COMPLETE (2026-06-28)

- Branch: kivo-video-p5a-contract-layer-001
- Final HEAD: 2e346ae
- CTest: 9/9 test suites PASS
- Exit criteria: 132 PASS + 2 DEFERRED (H13 soak, H14 performance - BLOCKED_RUNTIME)

P5 Stages:
- P5A: Contract Definition (37/37 families) - DONE (commit 2506fdd)
- P5B: P4 Input Bridge (11/11) - DONE (commit 443afa5)
- P5C: FFmpeg Demux Backend (12/12) - DONE (commit 331b999)
- P5D: Decoder Contract (9/9) - DONE (commit 20de985)
- P5E: Software Decode / Timing (10/10) - DONE (commit 986b49c)
- P5F: GPU Rendering (12/12) - DONE (commit 3494acd)
- P5G: D3D11VA / P010 / GPU (13/13) - DONE (commit c7d7cf5)
- P5H: Color/HDR/Protected Closure (15 PASS + 2 DEFERRED) - DONE (commit 488c483)
- P5S: Security Closure (13/13) - DONE (commit 2e346ae)

Key architecture: header-only fake runtimes, no production .cpp, no real FFmpeg/D3D11/WASAPI deps.
All implementations use P5A contract types + fake runtime headers for testing.

P5 is locked. P6 requires explicit user authorization.

## P6 Audio Media Plane Status (2026-06-29)
- P6A Contract Layer: CLOSED (commit 3d50ec3, 298 headers, 44/44 gates PASS)
- P6B Fake Runtime Bridge: CLOSED (commit 874a725, 22 headers, 22/22 exit criteria PASS)
- P6C FFmpeg Decode Backend: CLOSED (commit 9b5c334, 3/3 CTest PASS, 15 PASS + 1 DESIGNED E15 scope)
- P6D WASAPI Output Backend: CLOSED (REV 002 — start gap fixed with pre-fill)
- P6E Resampler/DSP/Clock/Sync: CLOSED (REV 002 — all 6 gaps fixed)
  - Real avcodec decode backend isolated in backend/src/video/audio_plane/decode/ffmpeg/
  - Guard updated: BackendPrivate mode allows P6C paths
  - FFmpeg SDK: C:/ffmpeg-sdk/ffmpeg-n7.1-latest-win64-gpl-shared-7.1
  - KIVO_ENABLE_FFMPEG=ON required for P6C tests
- P6D/P6E+: NOT STARTED (requires user authorization)

## P6 Status (2026-06-29, REV 002 — all gaps fixed)
- P6A Contract Layer: CLOSED
- P6B Fake Runtime Bridge: CLOSED
- P6C FFmpeg Decode Backend: CLOSED
- P6D WASAPI Output Backend: CLOSED (REV 002 — start gap fixed with pre-fill)
- P6E Resampler/DSP/Clock/Sync: CLOSED (REV 002 — all 6 gaps fixed: drift/quantize/noise shaping/IEC61937/gapless/device thread)
- P6S Security: CLOSED (in P6A)
- P6 is now CLOSED. All audit gaps resolved.

## Memory Maintenance
- Updated: 2026-06-26 (V8 rejection fix: E2E/soak tests now require ALL 4 hardware pre-conditions; D3D11/WASAPI missing → BLOCKED_ENV; P2 Foundation clearly separated from APRV-001 verification; GitHub Actions FFmpeg ON workflow added; evidence distinction enforced)