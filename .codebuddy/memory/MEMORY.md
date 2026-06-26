# Kivo Video Project Long-term Memory

## Project Overview
- **Repository**: https://github.com/syzayy520/kivo-video
- **Primary Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof
- **Current Focus**: P2 Local Runtime verified; next → P2-SCHEMA-GATE / P2-REDLINE-GATE / P2-SOURCE-CORE
- **Next Task**: P2-SCHEMA-GATE or P2-REDLINE-GATE (NOT P3)

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
- **Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof
- **HEAD**: efb417c (P2 Local Runtime Final Verification)
- **Remote**: Synced
- **Working Tree**: Clean
- **CTest**: 84/84 PASS (FFmpeg ON, golden sample)
- **Golden Sample**: kivo_h264_aac_35s_golden.mp4 (35s, H.264+AAC)
- **E2E**: RUNTIME_PASS (640x360, 3 D3D11, WASAPI)
- **Soak**: RUNTIME_PASS (35s, 10 D3D11, 3545 PCM)
- **Gate**: No Debug Dialog PASS
- **P2 Local Runtime**: RUNTIME_PASS
- **local_file provider**: RUNTIME_PASS (only provider with RUNTIME_PASS)
- **Push**: efb417c pushed to kivo-video-p2-real-001-repair-ffmpeg-proof

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

## Memory Maintenance
- Updated: 2026-06-26 (V8 rejection fix: E2E/soak tests now require ALL 4 hardware pre-conditions; D3D11/WASAPI missing → BLOCKED_ENV; P2 Foundation clearly separated from APRV-001 verification; GitHub Actions FFmpeg ON workflow added; evidence distinction enforced)