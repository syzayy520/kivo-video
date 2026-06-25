# Kivo Video Project Long-term Memory

## Project Overview
- **Repository**: https://github.com/syzayy520/kivo-video
- **Primary Branch**: kivo-video-p2-real-001-repair-ffmpeg-proof
- **Current Focus**: P2 Remaining Coverage V8 Global Gates
- **Next Task**: LRD-001 Planning Draft

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
12. **final_gate** (PFFG-001, PFFG-002, APRV-001): P2 foundation final gate, foundation final report, all providers runtime verified gate

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
- **HEAD**: V8-009 completion (all V8 gates DONE)
- **Remote**: Synced
- **Tests**: 84/84 CTest PASS (FFmpeg ON build)
- **Working Tree**: Clean
- **V8 Gates**: V8-001 through V8-009 all DONE, evidence in artifacts/p2/evidence/

## V8 Global Gates Completion
- **V8-001**: Branch / Workspace / Root Clean Gate - DONE
- **V8-002**: Current Checklist Consistency Gate - DONE
- **V8-003**: No Duplicate Work Gate - DONE
- **V8-004**: Natural Family Tree Gate - DONE
- **V8-005**: Allowed / Forbidden Claims Gate - DONE
- **V8-006**: Status Vocabulary Gate - DONE
- **V8-007**: Evidence Path Gate - DONE
- **V8-008**: Provider Runtime Matrix Gate Plan - DONE
- **V8-009**: Final Gate Sequencing Gate - DONE

## Memory Maintenance
- Updated: 2026-06-26 (V8-001 through V8-009 completion)