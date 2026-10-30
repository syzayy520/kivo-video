# P10 Real Playback Current State Handoff

Date: 2026-07-01

## Status

Current P10 baseline is correct.

- Workspace: `C:\kivo video`
- Branch: `codex/kivo-video-p0-ui-01-playback-overlay-v13`
- P10 current correct baseline HEAD: `e1394ff7d4315b502878b18a96963530213dbe6a`
- Current status: `PASS_CONTINUOUS_PLAYBACK_PUSHED`

Baseline correction:

- Old wrong baseline recorded here: `21881f95badce4be7c7ddb375f9c13c76286f7aa`
- Corrected baseline: `e1394ff7d4315b502878b18a96963530213dbe6a`
- Reason: `21881f` is the P10 first real frame commit. It has been superseded by
  `e1394ff`, the later continuous playback commit that was completed and pushed.

Do not reset, checkout, branch, clean, stash, or treat the current HEAD as a
handoff blocker. Future P10 V1 closure / real playback full closure work must
continue from `e1394ff7d4315b502878b18a96963530213dbe6a`.

## P10 Task Source

Attachment:

`C:\Users\huyasi\.codex\attachments\1f5f28af-f32b-45b1-bf46-96a548f0117d\pasted-text.txt`

Task id:

`KIVO-VIDEO-P10-REAL-PLAYBACK-PIPELINE-V1-CLOSURE-001`

Task target:

Turn real first-frame proof into real local video continuous playback V1.

Required verification sequence from the task, when future P10 closure work is
resumed:

```text
cmake --preset vs-debug-ffmpeg
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
cmake --build --preset vs-debug-ffmpeg --target kivo_player_runtime_adapter_tests --config Debug
ctest --preset vs-debug-ffmpeg -R "player_runtime_adapter|playback_shell|qml_load|ui_shell" --output-on-failure
build\vs-debug-ffmpeg\ui\Debug\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts\p10\continuous --media "<UNC Shelter 2026 directory>" --duration-ms 10000
```

Minimum proof required by the P10 continuous playback task:

- Continuous playback for at least 10 seconds.
- `decoded_frame_count > 1`.
- `rendered_frame_count > 1`.
- Timeline position advances at t0/t5/t10.
- Screenshots exist for t0, t5, t10/post-seek.
- Screenshot hashes or frame signatures prove frames are not repeated first frame.
- Pause holds timeline.
- Resume advances timeline.
- Seek produces a post-seek frame.
- Stop reaches a clear stopped/ready state.
- `fake_frame_used=false`.
- `fake_content_used=false`.
- Shell exits normally with no residual process.

## Scope Guard

For this P10 record correction, no business code, build, test, or UI changes were
requested.

For future P10 playback work, do not do unrelated UI visual polish unless the
user explicitly changes scope. The P10 playback task forbids:

- UI visual changes.
- PlaybackPage layout, font, spacing, or control structure changes.
- Playlist/window work.
- Network sources.
- HDR/Dolby/tone mapping.
- Advanced subtitle work.
- Fake frame or placeholder content.
- Direct QML calls to P4/P5/P6/P8 internals.
- Demux/decode/render logic inside shell.
- Playback policy inside QObject wrapper.

Correct route remains:

```text
Local / UNC file
-> Source open / SourceSession
-> P4 Playback Core
-> P5 Video Media Plane / FFmpeg demux decode
-> frame conversion if needed
-> render surface / video item
-> P7 PlaybackSession public API
-> PlayerRuntimeAdapter
-> P9 Shell wrapper
-> QML PlaybackPage visible surface
```

## Existing Structure Found

P9 shell:

- `ui/CMakeLists.txt`
- `ui/shell/playback_shell_main.cpp`
- `ui/shell/playback_shell_runtime_context.cpp`
- `ui/shell/playback_shell_runtime_adapter_bridge.cpp`
- `ui/shell/playback_shell_video_surface_item.cpp`
- `ui/shell/playback_shell_continuous_playback_evidence.cpp`

P9 closure:

- `docs/closure/p9_playback_shell_closure.md`
- Current claim there: P9 closed, ready for P10 real media open/playback path.

PlayerRuntimeAdapter:

- `backend/src/video/player_runtime_adapter/core/player_runtime_adapter.cpp`
- `backend/src/video/player_runtime_adapter/commands/adapter_lifecycle_commands.cpp`
- `backend/src/video/player_runtime_adapter/commands/adapter_playback_commands.cpp`
- `backend/src/video/player_runtime_adapter/commands/adapter_seek_commands.cpp`
- `backend/src/video/player_runtime_adapter/commands/adapter_track_commands.cpp`
- `backend/src/video/player_runtime_adapter/commands/adapter_policy_commands.cpp`
- `backend/src/video/player_runtime_adapter/commands/adapter_local_media_commands.cpp`
- `backend/src/video/player_runtime_adapter/diagnostics/adapter_diagnostics_commands.cpp`

P7 local media first-frame path:

- `backend/src/video/playback_graph/local_media/local_media_first_frame_pipeline.cpp`
- `backend/tests/video/playback_graph/p7h_local_media_first_frame_tests.cpp`
- CMake target: `kivo_p7h_local_media_first_frame_tests`

Continuous evidence path:

- `ui/shell/playback_shell_continuous_playback_evidence.cpp`
- Shell option: `--continuous-playback-evidence`

## Dirty/Untracked State Seen Before Correction

Tracked dirty files seen before this documentation correction:

- `artifacts/p9/evidence/ui_open_screenshot.png`
- `backend/tests/video/audio_plane/p6d_wasapi_output_tests/wasapi_output_backend_tests.cpp`
- `backend/tools/governance/redline_gate/artifacts/redline_gate_result.json`

Untracked paths seen before this documentation correction:

- `.codebuddy/teams/`
- `.codex/Kivo Video P7 Playback Graph Integration Design V1.9.1.docx`
- `agent-tools/`
- `artifacts/p10/continuous/`
- `artifacts/p10/playback_vsdebug/`
- multiple `artifacts/p9/evidence/*.txt`
- `artifacts/p9/interaction/`
- `terminals/`

Do not clean these automatically.

## 2026-07-01 Local Playback Wiring Update

Current local HEAD during this update:

- `2b5cfaa5f7f54bae36889cfc7887398d59af95b4`
- `HEAD_CONTAINS_E1394FF=yes`

P0/P1 audit:

- P0 docs found under `docs/p0/...`.
- P1 roadmap docs found under `docs/roadmap/p1_*.md`.
- P0 is not a playback data-plane implementation layer. It freezes the backend
  foundation: core states, versioned contracts, source identity, playback plan,
  capability layering, decision ledger, presenter/audio/subtitle contracts,
  inspector/diagnostic bundle contracts, governance CI gates, and P0/P1 landing
  route.
- P1 is not real playback. It is the fake backend / engine API / state machine /
  session ledger / replay-manifest proof layer that validates the foundation
  before later real playback phases.
- P0/P1 still apply to P10 local playback as hard governance constraints:
  no fake claims, no unregistered cross-module contract claims, no forbidden
  FFmpeg/WASAPI/Qt leaks into core or public adapter boundaries, evidence must
  match the real runtime, and unsupported backend capabilities must be reported
  as deferred instead of faked.
- Current P1 governance gate was attempted with bundled Python:
  `C:\Users\huyasi\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe backend\tools\governance\verify_p1_gates.py`
- Result: `FAIL: unexpected root-level file: cmake_p5d_build.log`.
- `cmake_p5d_build.log` is ignored/untracked and was last modified
  `2026-06-28 06:16:16`; it was not touched or cleaned during this task.

P0-P8 audit map for this checkout:

| Phase | Current audited role | Current real-playback status |
| --- | --- | --- |
| P0 | Foundation/governance contracts, schema, evidence, forbidden include gates. | Applies as a hard gate. Not a playback data plane. |
| P1 | Fake backend, engine API, state machine, replay/gate discipline. | Applies as fake-claim governance. `verify_p1_gates.py` currently fails on existing ignored `cmake_p5d_build.log`; no PASS claim. |
| P2 | Source Runtime for local/UNC/provider read/seek/close. | Connected for accessible local/UNC files through SourceSession-backed FFmpeg custom I/O. Shelter UNC provider kind is `smb`. |
| P3 | Source Runtime -> Playback-Ready Input System bridge: provider/transport/delivery/probe/decision/handoff/evidence/security. | Historical P3/P4 contract families were restored into this working tree and `kivo_p3_*` tests pass locally. Local media open now creates a P3 direct-play `PlaybackCandidateSet` / `PlaybackHandoffContract`, and P7 consumes its P4 open envelope. |
| P4 | Playback Core command/state/timeline semantics. | Historical P4 control-plane contract family was restored and `kivo_p4_*` tests pass locally. P4 open envelope from P3 is created, and completed P7 commands now commit P4 playback revision evidence. Full P4 state/timeline ownership is still not proven. |
| P5 | Video Media Plane contract family and production video runtime for demux/decode/render ownership. | P5A-P5S contract tests pass locally. A production `kivo_video_plane_local_media_runtime` target now owns SourceSession-backed first-frame probe/decode and continuous demux/video decode/RGBA for local/UNC playback. Hardware soak/performance and advanced GPU/HDR closure remain deferred. |
| P6 | Audio Plane. | Connected for local decoded PCM/WASAPI writes through audio-plane local PCM runtime; no bit-perfect/passthrough/exclusive/full A/V sync claim. |
| P7 | Playback Graph/session coordinator and public API. | Connected. QML/Adapter enter through P7 public PlaybackSession/local media facade. |
| P8 | Subtitle Plane. | Track discovery ids are exposed, but embedded subtitle packet/cue/render handoff is not connected; no fake subtitle playback claim. |

Implemented locally:

- FFmpeg runtime DLL deployment helper:
  `cmake/ffmpeg_runtime_deploy.cmake`
- FFmpeg DLL copy attached to backend adapter/probe/demux/decode/P6/P7 tests and
  the Qt playback shell target.
- PlayerRuntimeAdapter local media open now calls the P7 public
  `PlaybackSession::open_local_media_file(...)` facade instead of including P7
  local_media internals.
- P7 local_media continuous playback path now supports pause, resume, seek, and
  stop through P7 session commands.
- P2 SourceSession-backed input is connected for accessible local/UNC media:
  UNC paths are classified as `ProviderKind::smb`, and FFmpeg reads/seeks through
  P2 provider callbacks instead of opening the path directly.
- FFmpeg custom I/O is isolated behind the FFmpeg adapter opaque handle contract;
  no AVIO/AVFormatContext type is exposed to UI, Adapter, or P7 public headers.
- `--media` normal shell launch now opens a real media file through
  `PlayerRuntimeAdapter -> P7`.
- Shell evidence code was split into natural family files:
  evidence options, normal media launch, video launch support, and continuous
  evidence record writing, plus backend layer status reporting.
- UNC directory media resolution was hardened for paths containing spaces by
  using the original path plus a `std::filesystem` fallback.
- P6 local PCM audio runtime was added under the audio plane boundary and is
  driven from P7 local_media audio packet dispatch. It uses the real FFmpeg
  audio decode backend, converts frames to the actual WASAPI mix format, and
  writes PCM through `WasapiSharedPcmWriter`.
- Continuous evidence now records backend layer status explicitly, including
  P0/P1 governance, P2 SourceSession, P3 direct-play handoff/envelope
  consumption, P4 command lifecycle/revision evidence with deferred full state
  ownership, P5 local media production runtime ownership, P6 PCM/WASAPI
  connection, and P8 deferred subtitles.
- Historical P3/P4 contract families were restored into the current working
  tree without checkout/reset/switch:
  `backend/include/kivo/p3`, `backend/src/p3`, `backend/tests/p3`,
  `backend/include/kivo/playback/control_plane`,
  `backend/tests/playback/control_plane`, P3/P4 tools, architecture files,
  evidence packs, and closure/design docs.
- `backend/CMakeLists.txt` now registers restored P3/P4 contract/test targets:
  `kivo_p3_execution_family_test`, `kivo_p3_source_model_platform_test`,
  `kivo_p3_contracts_test`, `kivo_p3_behavior_test`,
  `kivo_p3_acceptance_test`, `kivo_p3_completion_test`,
  `kivo_p3_architecture_guard_scan`, `kivo_p4_primitives_revision_test`,
  `kivo_p4_revision_control_test`, `kivo_p4_reader_memory_buffer_test`,
  `kivo_p4_feedback_clock_track_test`,
  `kivo_p4_recovery_events_diagnostics_test`,
  `kivo_p4_output_boundary_test`, `kivo_p4_negative_compile_test`,
  `kivo_p4_behavior_test`, and `kivo_p4_architecture_guard_scan`.
- P7 local media now creates and consumes a P3/P4 handoff bridge:
  `LocalMediaP3HandoffBridge` builds a real local direct-play
  `PlaybackCandidateSet`, selected `PlaybackHandoffContract`, and P4
  `PlaybackOpenEnvelope` from the P2 SourceSession-backed probe status. P7
  requires the P4 open envelope before starting continuous local playback and
  records `p3_handoff_consumed_by_p7=true` when it starts.
- P2 and P5 runtime ownership has been split into explicit CMake targets:
  `kivo_video_source_runtime_local_media` owns the SourceSession-backed local/UNC
  input callbacks, and `kivo_video_plane_local_media_runtime` owns first-frame
  probe/decode plus continuous demux/video decode/RGBA. P7 now coordinates these
  runtimes and no longer contains FFmpeg/D3D/swscale/KivoPacket implementation
  symbols under `backend/src/video/playback_graph` or
  `backend/include_private/video/playback_graph`.

Verification run locally:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
cmake --build --preset vs-debug-ffmpeg --target kivo_player_runtime_adapter_tests --config Debug
cmake --build --preset vs-debug-ffmpeg --target kivo_video_provider_local_file_open_test kivo_video_provider_local_file_read_test kivo_video_provider_local_file_zero_read_test kivo_video_provider_local_file_seek_test kivo_video_provider_local_file_error_test kivo_video_provider_local_file_close_test kivo_video_provider_local_file_redaction_test kivo_video_provider_local_file_evidence_test kivo_video_provider_local_file_direct_input_test kivo_video_provider_local_file_path_validation_test kivo_video_provider_local_file_schema_test kivo_video_provider_local_file_basic_thread_safety_test --config Debug
ctest --preset vs-debug-ffmpeg -R "player_runtime_adapter|playback_shell|qml_load|ui_shell|provider_local_file" --output-on-failure
kivo_video_playback_shell.exe --media "<UNC Shelter 2026 directory>"
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Latest verification run locally after P6 wiring:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "player_runtime_adapter|playback_shell|qml_load|ui_shell|provider_local_file|p6c|p6e|decoded_audio_frame_converter|wasapi_shared_pcm_writer" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Latest verification run locally after track/subtitle fake-risk cleanup:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "p7c_command_state|player_runtime_adapter|playback_shell|qml_load|ui_shell|provider_local_file|p6c|p6e|decoded_audio_frame_converter|wasapi_shared_pcm_writer" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Latest verification run locally after Adapter/QML track inventory exposure:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_player_runtime_adapter_tests --config Debug
ctest --preset vs-debug-ffmpeg -R "p7c_command_state|player_runtime_adapter|playback_shell|qml_load|ui_shell|provider_local_file|p6c|p6e|decoded_audio_frame_converter|wasapi_shared_pcm_writer" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Tests: `26/26` passed.
- Runtime evidence: passed with `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- Boundary scan: no UI/Adapter direct FFmpeg/WASAPI/internal references; no P7/Shell direct
  `avformat_open_input` or direct `open_file` route found.
- P1 gate: still `FAIL: unexpected root-level file: cmake_p5d_build.log`.

Latest verification run locally after P4 partial evidence refinement:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "p7c_command_state|player_runtime_adapter|playback_shell|qml_load|ui_shell|provider_local_file|p6c|p6e|decoded_audio_frame_converter|wasapi_shared_pcm_writer" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Tests: `26/26` passed.
- Runtime evidence: passed with `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- P4 partial evidence: `p7_retained_command_count=5`,
  `p4_playback_revision=0`, `p4_command_lifecycle_connected=true`,
  `p4_playback_revision_committed=false`.
- P1 gate: still `FAIL: unexpected root-level file: cmake_p5d_build.log`.

Latest verification run locally after P4 revision commit wiring:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_p7c_command_state_tests kivo_player_runtime_adapter_tests --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Tests: `20/20` passed.
- Runtime evidence: passed with `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- P4 revision evidence: `p4_playback_revision=5`,
  `p4_playback_revision_committed=true`.
- P1 gate: still `FAIL: unexpected root-level file: cmake_p5d_build.log`.

Latest verification run locally after P5 local media production runtime split:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_p7c_command_state_tests kivo_player_runtime_adapter_tests --config Debug
cmake --build --preset vs-debug-ffmpeg --target kivo_video_plane_contract_tests kivo_video_plane_input_bridge_tests kivo_video_plane_ffmpeg_demux_tests kivo_video_plane_decoder_tests kivo_video_plane_timing_tests kivo_video_plane_gpu_render_tests kivo_video_plane_d3d11va_p010_tests kivo_video_plane_color_hdr_protected_tests kivo_video_plane_security_closure_tests --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_video_plane_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
rg -n "FfmpegAdapter|RealSoftwareDecodeRuntime|KivoPacket|libav|D3d11DeviceContext|D3d11TextureUpload|convert_decoded_frame|swscale|avformat|avcodec" backend/src/video/playback_graph backend/include_private/video/playback_graph
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Tests: `29/29` passed.
- Boundary scan: no FFmpeg/D3D/swscale/KivoPacket implementation symbols remain
  in P7 playback graph source/private headers.
- Runtime evidence: passed with `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- P5 evidence: `p5_local_video_runtime_connected=true`,
  `p5_production_runtime_target_present=true`,
  `p5_formal_video_plane_connected=true`,
  `p5_audio_packet_boundary_connected=true`,
  `p5_audio_packets_dispatched=10818`.
- P1 gate: still `FAIL: unexpected root-level file: cmake_p5d_build.log`.

Latest local evidence:

- `artifacts/local_playback/continuous_playback_evidence.json`
- `artifacts/local_playback/real_playback_local_evidence.json`
- `artifacts/local_playback/playback_t0.png`
- `artifacts/local_playback/playback_t5s.png`
- `artifacts/local_playback/playback_t10s.png`

Evidence result:

- `continuous_playback_evidence.json`: `PASS_CONTINUOUS_PLAYBACK`
- `source_session_connected`: `true`
- `source_provider_kind`: `smb`
- `container`: `matroska,webm`
- `video_codec`: `hevc`
- `decoded_frame_count`: `79`
- `rendered_frame_count`: `79`
- `audio_codec`: `truehd`
- `audio_decode_started`: `true`
- `audio_output_started`: `true`
- `audio_frames_decoded`: `4182`
- `audio_frames_written`: `2625`
- `video_stream_count`: `4`
- `audio_stream_count`: `3`
- `subtitle_stream_count`: `5`
- timeline advanced to about `80885ms`
- pause held timeline
- resume advanced timeline
- seek accepted and post-seek frame evidence passed
- stop reached `ready`
- `fake_frame_used=false`
- `fake_content_used=false`

Local real playback classification:

- `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`
- `audio_status`: `connected_pcm_wasapi`
- `subtitle_status`: `deferred`

2026-07-02 P0-P8 correction and P3/P4 handoff/revision verification:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Latest parsed evidence after P3/P4 handoff and P4 revision connection:

- `continuous_classification`: `PASS_CONTINUOUS_PLAYBACK`
- `real_classification`: `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`
- `source_session_connected`: `true`
- `source_provider_kind`: `smb`
- `decoded_frame_count`: `79`
- `rendered_frame_count`: `79`
- `audio_codec`: `truehd`
- `audio_frames_written`: `2472`
- `video_stream_count`: `4`
- `audio_stream_count`: `3`
- `subtitle_stream_count`: `5`
- `fake_frame_used`: `false`
- `fake_content_used`: `false`
- `p0_contract_governance_applies`: `true`
- `p1_fake_backend_governance_applies`: `true`
- `p1_governance_gate`: `FAIL: verify_p1_gates.py stops on existing ignored root-level cmake_p5d_build.log`
- `p2_source_session_connected`: `true`
- `p3_candidate_set_created`: `true`
- `p3_handoff_created`: `true`
- `p3_handoff_consumed_by_p7`: `true`
- `p4_open_envelope_from_p3_created`: `true`
- `p4_playback_revision`: `5`
- `p3_candidate_kind`: `direct_play`
- `p3_playback_ready_input_bridge_connected`: `true`
- `p4_control_plane_contract_family_present`: `true`
- `p4_control_plane_contract_tests_passed_locally`: `true`
- `p4_command_lifecycle_connected`: `true`
- `p4_playback_revision_committed`: `true`
- `p5_contract_family_present`: `true`
- `p5_production_runtime_target_present`: `true`
- `p5_formal_video_plane_connected`: `true`
- `p5_local_video_runtime_connected`: `true`
- `p5_audio_packet_boundary_connected`: `true`
- `p5_audio_packets_dispatched`: `10818`
- `p6_audio_connected`: `true`
- `p8_subtitle_connected`: `false`

2026-07-02 P8 truthful evidence refinement:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_player_runtime_adapter_tests --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Tests: `20/20` passed.
- Runtime evidence: `PASS_CONTINUOUS_PLAYBACK` and
  `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- `source_provider_kind`: `smb`
- `decoded_frame_count`: `79`
- `rendered_frame_count`: `79`
- `audio_codec`: `truehd`
- `audio_frames_written`: `2617`
- `subtitle_stream_count`: `5`
- `subtitle_status`: `deferred`
- `p8_contract_family_present`: `true`
- `p8_subtitle_track_inventory_discovered`: `true`
- `p8_subtitle_connected`: `false`
- `p8_embedded_packet_ingest_connected`: `false`
- `p8_text_buffer_fixture_only`: `true`
- `p8_pgs_bitmap_render_connected`: `false`
- `layer_status.p8_real_local_bridge_attached`: `false`

P8 audit detail:

- `include/kivo/video/subtitle_plane/api/subtitle_plane_api_v1.h` exposes
  source/frame/snapshot/text-buffer APIs such as `set_source`, `build_frame`,
  `copy_snapshot`, and `copy_text_buffer`; it does not expose a real embedded
  subtitle packet/cue submission API for the local media playback path.
- `include/kivo/video/subtitle_plane/source/subtitle_source.h` includes
  `KivoSubtitleSourceKind_EmbeddedTrack`, but the current runtime source API
  only validates/probes a source and marks the plane ready. It does not ingest
  demuxed embedded subtitle packets into an active cue timeline.
- `backend/src/video/subtitle_plane/text/text_buffer_store.cpp` currently
  returns the fixed contract text `cue` for text buffer id 1. That is acceptable
  only as contract/fixture behavior and must not be surfaced as real local/UNC
  subtitle playback.
- `backend/src/video/playback_graph/p8_bridge/subtitle_frame_p8_bridge.cpp`
  only attaches its memory cue fixture when `allow_contract_fixture_source` is
  true. Real local/UNC open sets that flag false, so P8 frame query remains
  unavailable instead of faking subtitle text.
- The Shelter file exposes 5 `hdmv_pgs_subtitle` streams. There is no current
  PGS bitmap subtitle decode/render handoff into P8 or QML overlay, so P8 stays
  deferred truthfully.

2026-07-02 P4 timeline contract bridge refinement:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_p7c_command_state_tests kivo_player_runtime_adapter_tests --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Tests: `20/20` passed.
- Runtime evidence: `PASS_CONTINUOUS_PLAYBACK` and
  `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- Added `P4TimelineContractBridge` under the existing P7 private
  `p4_bridge` family. It maps the current P7 local playback snapshot into P4
  public `PlaybackTimelineState` contract types.
- Added a P7C ownership test that requires:
  `p4_timeline_contract_connected=true`,
  `p4_timeline_uses_public_contract=true`, and
  `p4_runtime_state_machine_owner=false`.
- Latest parsed evidence:
  `p4_playback_revision=5`,
  `p4_timeline_contract_connected=true`,
  `p4_timeline_uses_public_contract=true`,
  `p4_runtime_state_machine_owner=false`,
  `p4_timeline_revision=11`,
  `p4_timeline_generation=5`,
  `p4_timeline_observed_position_ms=81027`,
  `p4_timeline_duration_ms=6435179`.

This is a P4 public contract bridge, not a full P4 runtime ownership claim. Full
P4 state-machine/timeline ownership remains deferred until a real P4 control
runtime owns those transitions instead of P7 deriving a contract snapshot.

2026-07-02 playback shell UI/evidence polish:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
kivo_video_playback_shell.exe --ui-open artifacts/p10/ui_open_alias --ui-open-close-ms 500
ctest --preset vs-debug-ffmpeg -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
```

Result:

- Build: passed.
- `--ui-open` is now accepted as a compatibility alias for the existing
  `--ui-open-evidence` option. The alias generated
  `artifacts/p10/ui_open_alias/ui_open_screenshot.png` and exited normally.
- Shell QML tests: `2/2` passed.
- `PlaybackShellRoot.qml` now uses a frameless black playback window instead of
  the default Windows title-bar frame.
- `PlaybackIconButton.qml` no longer draws rounded rectangular button plates for
  normal playback controls; it uses icon-first controls with a subtle hover
  underline.

Track/subtitle fake-risk cleanup:

- Local/UNC media now populates P7 track inventory from real probed stream
  indices instead of the P7 contract-fixture defaults.
- The P7 contract-fixture track inventory remains available for non-local fake
  contract tests only.
- `SubtitleFrameP8Bridge` no longer attaches its memory cue fixture for real
  local/UNC media. If embedded subtitle source/frame handoff is not connected,
  subtitle frame query reports unavailable/deferred instead of returning fake
  text.
- The latest Shelter evidence records real stream counts from the media:
  `video_stream_count=4`, `audio_stream_count=3`, `subtitle_stream_count=5`.
- PlayerRuntimeAdapter now exposes P7 track inventory ids to QML snapshot fields:
  `audioTrackOptions`, `subtitleOptions`, `selectedAudioTrack`, and
  `selectedSubtitleTrack`. Existing QML audio/subtitle sheets consume these
  options through `PlaybackSnapshotBinding`.
- Track option labels are generic id labels only; no language, channel layout,
  title, or subtitle content metadata is fabricated.
- P8 audit found no public API for submitting embedded subtitle packets/cues into
  the subtitle plane. Current P8 frame/text buffer behavior still contains
  contract fixtures for non-real tests, so real local/UNC playback must keep P8
  subtitle frame rendering deferred until a real packet/cue input API and bitmap
  PGS path exist.
- P4 status is now reported as partial, not absent: P7 command lifecycle
  allocates P4 command ids, retains command records, and completed accepted
  commands commit P4 playback revision evidence. Full P4 state-machine/timeline
  ownership is still not claimed.

Important deferred backend wiring:

- P5 now has a production local media runtime target for this local/UNC playback
  path, and P7 no longer owns FFmpeg/D3D/swscale video implementation code. This
  is not a full P5 hardware-lab closure claim: P5 H13/H14 soak/performance,
  advanced GPU/HDR policy, full D3D11VA/P010 production path, and long-run
  hardware validation remain deferred.
- P6 audio is connected for decoded PCM write through the audio-plane local PCM
  runtime and WASAPI writer. This is not yet a bit-perfect, passthrough, or full
  A/V sync closure claim.
- P8 subtitle discovery/render handoff is not connected to this path; no
  subtitle playback claim.

Backend gap audit detail:

- P0/P1 are active gates for this task. They are not bypassed because they are
  not playback data planes; they govern contract boundaries, fake-claim safety,
  dependency exposure, schema/evidence wording, and whether a later task may
  claim PASS.
- P1 gate was run and is currently `FAIL` on existing ignored root-level
  `cmake_p5d_build.log`. This task did not clean or delete that non-task file,
  so P1 remains a recorded governance blocker, not a PASS.
- P2 SourceSession-backed input is now observed in the real playback evidence.
  For the Shelter UNC path, provider kind is `smb`. This means accessible UNC
  files can be read through P2 provider read/seek callbacks, but it does not
  claim a complete SMB auth/credential/reconnect backend.
- P3 exists and was historically closed with accepted deferrals on commit
  `791805e1378bbe675608c67cb01c40d9dd051ef9`, according to project memory and
  git history. P3 is the `Source Runtime -> Playback-Ready Input System` bridge:
  provider/transport/delivery/probe/decision/handoff/evidence/security producing
  playback-ready input candidates for P4. It is explicitly not renderer,
  decoder, audio output, subtitle bitmap, UI, or playback timeline control.
- Current HEAD `2b5cfaa5f7f54bae36889cfc7887398d59af95b4` was not a descendant
  of the historical P3/P4 closure commits, so the P3/P4 contract families were
  missing from this branch. They have now been restored into the working tree
  from the historical baseline and registered in CMake. Verification:
  `ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_" --output-on-failure`
  is `16/16 PASS`.
- Runtime P3 consumption is now connected for the local/UNC direct-play path:
  local open creates a P3 `PlaybackCandidateSet` and
  `PlaybackHandoffContract`, creates a P4 `PlaybackOpenEnvelope`, and P7 marks
  that envelope consumed before continuous playback starts. This is a
  direct-play local handoff claim only; it does not claim remote provider
  ranking, manifest/HLS/DASH runtime, reconnect/auth handling, or P4 full state
  ownership.
- P5 input/video ownership is now connected for the local/UNC path through
  `kivo_video_plane_local_media_runtime`. The route reaches FFmpeg through P5
  runtime files under `backend/src/video/video_plane/runtime/local_media`, with
  SourceSession callbacks supplied by `kivo_video_source_runtime_local_media`.
  Evidence records `p5_contract_family_present=true`,
  `p5_production_runtime_target_present=true`, and
  `p5_formal_video_plane_connected=true`. This does not claim P5 hardware
  soak/performance closure or advanced HDR/GPU production readiness.
- P6 local PCM wiring is now observed in the real playback evidence: TrueHD
  packets are dispatched from P7 local_media into the P6 audio runtime, decoded
  with the real FFmpeg audio backend, converted to the active WASAPI mix format,
  and accepted by the WASAPI PCM writer. Full bitstream passthrough, exclusive
  mode policy, and complete A/V clock ownership remain deferred.
- P8 has subtitle plane and P7 bridge pieces, but embedded subtitle discovery and
  active subtitle frame feed are not attached to this local media playback path.
- P4 full state ownership is not proven here. The evidence proves P7 session
  command semantics for open/play/pause/resume/seek/stop plus P4 command
  lifecycle/revision commits, not complete P4 state-machine/timeline ownership.

Do not claim full local playback closure from this update. The truthful local
state is real video playback through QML -> PlayerRuntimeAdapter -> P7 with P2
SourceSession-backed input, P3 local direct-play handoff consumption, P4 command
lifecycle/revision evidence, P5 local media production video runtime ownership,
and P6 decoded PCM/WASAPI audio writes, while P8 subtitles, full P4
state/timeline ownership, P5 hardware soak/performance/HDR closure, and advanced
audio policy remain deferred/current-branch gaps.

2026-07-02 window/fullscreen policy wiring update:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_p7c_command_state_tests kivo_player_runtime_adapter_tests kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/window_screen_interaction --interaction-close-ms 300
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Focused P7/Adapter/Shell tests: `4/4` passed.
- P0-P8/P10 focused set: `20/20` passed.
- New interaction evidence:
  `artifacts/p10/window_screen_interaction/interaction_evidence.json`.
- `seek` via QML evidence is now `accepted`; the evidence bridge now reads the
  QML return value as `QVariant` instead of an incorrect C++ `bool` signature.
- `enterFullscreen` via `PlaybackShellRuntimeAdapterBridge` is now `accepted`,
  not `typed-only`.
- Adapter snapshot now reports `fullscreenLike=true` after the accepted command.
- Real Shelter SMB continuous playback still records:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `decoded_frame_count=79`,
  `rendered_frame_count=79`,
  `audio_frames_written=2606`,
  `source_provider_kind=smb`,
  `source_session_connected=true`,
  `p3_handoff_consumed_by_p7=true`,
  `p4_timeline_contract_connected=true`,
  `p4_runtime_state_machine_owner=false`,
  `p5_local_video_runtime_connected=true`,
  `audio_output_started=true`,
  `p8_subtitle_connected=false`,
  `fake_frame_used=false`,
  `fake_content_used=false`,
  `errors=""`.

Implemented route:

```text
QML PlayerPage fullscreen button / F shortcut
-> PlaybackShellRuntimeAdapterBridge
-> PlayerRuntimeAdapter
-> P7 PlaybackSession window/screen policy
-> Shell applies QQuickWindow showFullScreen/showNormal only after Accepted
```

Implementation notes:

- Added the P7 public window/screen policy contract:
  `include/kivo/video/playback_graph/command/window_screen_policy_request.hpp`.
- Added P7 session/runtime methods:
  `enter_fullscreen()`, `exit_fullscreen()`, `move_to_screen(...)`, and
  `query_window_screen_policy()`.
- Added P7 runtime family file:
  `backend/src/video/playback_graph/session/playback_session_window_screen_policy.cpp`.
- Added `GraphCommandKind::SetWindowScreenPolicy`; it is accepted in any
  non-closed graph state and rejected after close by the existing closed-graph
  gate.
- PlayerRuntimeAdapter no longer returns
  `AdapterMissingP7Api::FullscreenWindowScreen` for fullscreen/window/screen
  commands. It calls the P7 public session API and maps window policy into
  `AdapterSnapshot::fullscreen_like`, `screen_index`, and
  `connections.fullscreen_screen_policy=ConnectedToP7`.
- Shell bridge owns the Qt window application step, not playback policy. It
  binds the `QQuickWindow`, syncs it from the Adapter snapshot, and applies
  `showFullScreen()`, `showNormal()`, or `setScreen(...)` only after the
  Adapter/P7 command is accepted.
- QML still only talks to the bridge/runtime adapter. It does not directly own
  a backend truth state and does not call P4/P5/P6/P8 internals.

Remaining truthful gaps:

- `move_to_screen` records and applies screen index when called, but there is no
  HDR mode / refresh-rate / monitor-color policy backend yet. Do not claim HDR
  screen policy closure from this update.
- Playlist/next episode was still missing at this window/screen update point;
  see the later 2026-07-02 playlist policy update below for the connected P7
  policy boundary and remaining real-queue gap.
- P8 embedded subtitle packet/cue/render handoff remains deferred.

2026-07-02 settings policy overlay wiring update:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/settings_interaction --interaction-close-ms 300
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Shell QML/interaction tests: `2/2` passed.
- P0-P8/P10 focused set: `20/20` passed.
- Settings interaction evidence:
  `artifacts/p10/settings_interaction/interaction_evidence.json`.
- Settings policy commands are accepted through the bridge pathway:
  `setPlaybackSpeed`, `setAspectMode`, `setToneMappingMode`, and
  `setAudioDelay`.
- `snapshot_before_close` records backend policy values returned through the
  Adapter/P7 snapshot:
  `playbackSpeed=1.25`,
  `aspectMode=fill`,
  `toneMappingMode=sdr`,
  `audioDelayMs=80`,
  `subtitleSize=1`,
  `fullscreenLike=true`.
- Real Shelter SMB continuous playback still records:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `decoded_frame_count=79`,
  `rendered_frame_count=79`,
  `audio_frames_written=2625`,
  `source_provider_kind=smb`,
  `source_session_connected=true`,
  `p3_handoff_consumed_by_p7=true`,
  `p4_timeline_contract_connected=true`,
  `p4_runtime_state_machine_owner=false`,
  `p5_local_video_runtime_connected=true`,
  `audio_output_started=true`,
  `p8_subtitle_connected=false`,
  `fake_frame_used=false`,
  `fake_content_used=false`,
  `errors=""`.

Implemented route:

```text
QML SettingsSheet / PlaybackOverlay
-> PlaybackPage dispatchRuntimeCommand
-> PlaybackShellRuntimeAdapterBridge
-> PlayerRuntimeAdapter policy commands
-> P7 PlaybackSession playback/subtitle policy
-> Adapter snapshot
-> QML PlaybackSnapshotBinding display state
```

Implementation notes:

- Added a focused settings sheet family under `ui/playback/sheets`:
  `SettingsSheet.qml`, `SettingStepperRow.qml`, and
  `SettingSegmentRow.qml`.
- The settings sheet exposes real policy controls for playback speed, aspect
  mode, tone mapping mode, audio delay, and subtitle scale. It does not create a
  local QML-only truth state for playback policy.
- `PlaybackShellRuntimeAdapterBridge` exposes policy methods for
  `setPlaybackSpeed`, `setAspectMode`, `setToneMappingMode`, and
  `setAudioDelay`, and maps string UI modes to P7 public policy enums.
- `playback_shell_snapshot_mapper.cpp` now publishes policy snapshot fields
  such as `playbackSpeed`, `aspectMode`, `toneMappingMode`,
  `audioDelayMs`, and `subtitleSize` to QML.
- `PlaybackSnapshotBinding.qml` consumes those fields from the backend snapshot.
- `SubtitleSheet.qml` no longer uses a fake pixel-sized subtitle value for real
  display state. It sends scale requests and relies on the backend snapshot for
  the current value.

Remaining truthful gaps:

- Playlist/next episode is connected to a P7 policy boundary as of the later
  2026-07-02 playlist update below. Real queue/session item data is still not
  connected, so do not claim playable next episode orchestration.
- HDR screen/refresh-rate policy is still not connected; the window/screen
  route only covers fullscreen-like state and screen index.
- P8 embedded subtitle packet/cue/render handoff is still deferred. Subtitle
  track ids may be discovered, but real P8 subtitle playback must not be
  claimed until packet/cue ingest and render handoff exist.
- Full P4 state-machine/timeline ownership is still deferred. The current claim
  remains P4 command lifecycle/revision evidence plus public timeline contract
  mapping, not complete P4 runtime ownership.

2026-07-02 playlist / next episode policy boundary update:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_p7c_command_state_tests kivo_player_runtime_adapter_tests kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/playlist_interaction --interaction-close-ms 300
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Focused P7/Adapter/Shell tests: `4/4` passed.
- P0-P8/P10 focused set: `20/20` passed.
- Playlist interaction evidence:
  `artifacts/p10/playlist_interaction/interaction_evidence.json`.
- `previous`, `next`, and `playNextEpisode` now travel through
  `PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7
  PlaybackSession`. They are no longer `AdapterMissingP7Api::Playlist`.
- Because no real queue/session item source is connected in this local single
  media path, those navigation commands are rejected by P7 with
  `PolicyUnsupportedByCapability`; UI evidence records them as rejected instead
  of typed-only or accepted.
- `setAutoPlayNext(true)` is accepted through the bridge and Adapter/P7 policy
  path. `snapshot_before_close` records:
  `autoPlayNext=true`,
  `playlistQueueConnected=false`,
  `playlistHasPrevious=false`,
  `playlistHasNext=false`,
  `playlistHasNextEpisode=false`,
  `playlistItemCount=0`.
- Real Shelter SMB continuous playback still records:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `decoded_frame_count=79`,
  `rendered_frame_count=79`,
  `audio_frames_written=2556`,
  `source_provider_kind=smb`,
  `source_session_connected=true`,
  `p3_handoff_consumed_by_p7=true`,
  `p4_timeline_contract_connected=true`,
  `p4_runtime_state_machine_owner=false`,
  `p5_local_video_runtime_connected=true`,
  `audio_output_started=true`,
  `p8_subtitle_connected=false`,
  `fake_frame_used=false`,
  `fake_content_used=false`,
  `errors=""`.

Implemented route:

```text
QML PlaybackPage / SettingsSheet
-> PlaybackShellRuntimeAdapterBridge
-> PlayerRuntimeAdapter playlist methods
-> P7 PlaybackSession playlist policy
-> Adapter snapshot
-> QML PlaybackSnapshotBinding display state
```

Implementation notes:

- Added the P7 public playlist policy contract:
  `include/kivo/video/playback_graph/command/playlist_policy_request.hpp`.
- Added P7 public session methods:
  `next()`, `previous()`, `play_next_episode()`, `set_auto_play_next(...)`, and
  `query_playlist_policy()`.
- Added P7 runtime family file:
  `backend/src/video/playback_graph/session/playback_session_playlist_policy.cpp`.
- Added `GraphCommandKind::PlaylistNavigate` and
  `GraphCommandKind::SetPlaylistPolicy`.
- PlayerRuntimeAdapter now calls P7 for playlist/navigation policy instead of
  returning `AdapterMissingP7Api::Playlist`.
- Adapter/QML snapshot now exposes:
  `autoPlayNext`, `playlistQueueConnected`, `playlistHasPrevious`,
  `playlistHasNext`, `playlistHasNextEpisode`, and `playlistItemCount`.
- `SettingsSheet` now includes an `Auto next` toggle that calls
  `setAutoPlayNext`; it does not fabricate episode options.

Remaining truthful gaps:

- A real queue/session item provider is not connected yet. Current local single
  media playback can expose policy state and correctly reject navigation, but it
  cannot play a real next/previous item until a P7 queue contract is backed by
  real P2/P3 media identity/session data.
- Episode sheet data remains empty unless a real backend queue supplies episode
  options. Do not add fake episode lists.
- P8 embedded subtitle packet/cue/render handoff remains deferred.

2026-07-02 buffering/opening snapshot UI wiring update:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --preset vs-debug-ffmpeg -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/buffering_snapshot_interaction --interaction-close-ms 300
ctest --preset vs-debug-ffmpeg -R "kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 30000 --continuous-close-ms 500
```

Result:

- Build: passed.
- Shell QML/interaction tests: `2/2` passed.
- P0-P8/P10 focused set: `20/20` passed.
- Buffering/opening interaction evidence:
  `artifacts/p10/buffering_snapshot_interaction/interaction_evidence.json`.
- `snapshot_before_close` now carries:
  `playbackState=Ready`,
  `isOpening=false`,
  `isBuffering=false`,
  `bufferReason=none`,
  `rebufferCount=0`.
- `PlaybackShellSnapshotMapper` now exposes `isOpening`, `bufferReason`,
  `rebufferCount`, and `playbackState` from the Adapter snapshot to QML.
- `PlaybackSnapshotBinding.qml` consumes these fields, and `PlaybackPage.qml`
  treats either `isOpening` or `isBuffering` as the loading/status overlay
  condition.
- Real Shelter SMB continuous playback still records:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `decoded_frame_count=79`,
  `rendered_frame_count=79`,
  `audio_frames_written=2630`,
  `source_provider_kind=smb`,
  `source_session_connected=true`,
  `p3_handoff_consumed_by_p7=true`,
  `p4_timeline_contract_connected=true`,
  `p4_runtime_state_machine_owner=false`,
  `p5_local_video_runtime_connected=true`,
  `audio_output_started=true`,
  `p8_subtitle_connected=false`,
  `fake_frame_used=false`,
  `fake_content_used=false`,
  `errors=""`.

Truthful limit:

- This is UI snapshot/status wiring for Adapter-provided
  opening/buffering/rebuffer fields. It does not prove a new backend rebuffer
  runtime path, network reconnect loop, or synthetic rebuffer scenario.

2026-07-02 local directory playlist queue wiring update:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_source_runtime_local_media_queue_tests --config Debug
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_video_source_runtime_local_media_queue_tests|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests" --output-on-failure
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_playback_shell_qml_load_test kivo_playback_shell_interaction_test --config Debug
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/local_playback --media "<UNC Shelter 2026 directory>" --duration-ms 10000 --continuous-close-ms 500
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_video_source_runtime_local_media_queue_tests|kivo_p3_|kivo_p4_|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_video_plane_.*tests|kivo_audio_plane_contract_tests|kivo_p6b_|kivo_p6c_|kivo_p6d_|kivo_p6e_|kivo_p8" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
```

Result:

- P2 directory queue unit test plus P7/Adapter focused tests: `3/3` passed.
- Shell QML/interaction tests: `2/2` passed.
- P3/P4/P7/Adapter plus queue focused set: `19/19` passed.
- P5/P6/P8 plane/contract set: `40/40` passed after building previously
  missing P6/P8 test executables.
- Real Shelter SMB continuous playback evidence:
  `artifacts/local_playback/continuous_playback_evidence.json`.
- Real screenshot reviewed:
  `artifacts/local_playback/playback_t10s.png`.

Real Shelter evidence fields:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `decoded_frame_count=79`
- `rendered_frame_count=79`
- `timeline_position_before_ms=509`
- `timeline_position_after_ms=64354`
- `source_provider_kind=smb`
- `source_session_connected=true`
- `p3_handoff_consumed_by_p7=true`
- `p4_timeline_contract_connected=true`
- `p5_local_video_runtime_connected=true`
- `audio_output_started=true`
- `audio_frames_written=2651`
- `subtitle_stream_count=5`
- `p8_subtitle_connected=false`
- `playlist_queue_connected=true`
- `playlist_item_count=1`
- `playlist_current_index=0`
- `playlist_has_previous=false`
- `playlist_has_next=false`
- `errors=""`

Implemented route:

```text
QML EpisodeSheet / PlaybackPage
-> PlaybackShellRuntimeAdapterBridge::playPlaylistItem(index)
-> PlayerRuntimeAdapter::play_playlist_item(index)
-> P7 PlaybackSession playlist policy
-> P2 local media directory queue discovery
-> P7 open_local_media_file(...) for selected queue item
```

Implementation notes:

- Added the P2 local media queue family:
  `backend/include_private/video/source_runtime/local_media/local_media_directory_queue.hpp`
  and
  `backend/src/video/source_runtime/local_media/local_media_directory_queue.cpp`.
- `build_local_media_directory_queue(...)` discovers real sibling video files
  from the current local/UNC media directory, validates each file through the
  local-file provider probe, sorts by filename, and marks the current file.
- P7 stores queue item paths privately and exposes only public snapshot data:
  item count, current index, has previous/next, and display labels.
- Adapter snapshot now maps P7 playlist items into `episodeOptions`; QML only
  displays backend-supplied items and sends selected `itemIndex` back through
  the bridge.
- Continuous evidence now records playlist queue fields and real item labels
  from the Adapter snapshot.
- The previous `playlistQueueConnected=false` note is superseded for local/UNC
  media directories. The Shelter test directory currently contains one real
  playable item, so `playlist_has_next=false` is a truthful queue state, not a
  missing policy boundary.

Boundary checks:

- UI/Adapter dependency search found no direct `avformat`, `avcodec`,
  `avutil`, `swscale`, `swr`, `mpv`, `QProcess`, or process-launch playback
  shortcut. The only UI hits were the runtime DLL deploy CMake helper and a
  backend-layer status string mentioning WASAPI.
- QML still does not call P4/P5/P6/P8 internals and does not fabricate episode
  options.
- P8 remains truthful deferred for embedded subtitle packet/cue/render handoff.
  Shelter exposes subtitle streams, but real P8 subtitle rendering is still not
  connected.

2026-07-02 UI open alias and real track-switch wiring update:

Current local HEAD during this update:

- `2b5cfaa5f7f54bae36889cfc7887398d59af95b4`
- Branch: `codex/kivo-video-p0-ui-01-playback-overlay-v13`

Implemented locally:

- Fixed the shell CLI parser so both `--ui-open` and single-dash `-ui-open`
  long-option style are accepted by `kivo_video_playback_shell`.
- Added real P5 local media stream codec metadata for video/audio/subtitle
  streams so P7 can restart the formal local media runtime on track switch
  without guessing codec names in UI or Adapter.
- Added initial seek support to the P5 local continuous runtime open request so
  P7 can restart the local media runtime near the current position during a real
  audio/video track switch.
- Moved audio/video track switching into a dedicated P7 session family file:
  `backend/src/video/playback_graph/session/playback_session_track_switch.cpp`.
- P7 now handles `switch_audio_track`, `switch_video_track`, and
  `switch_av_track_set` by validating P7 command state first, validating track
  ids second, and restarting the local P5/P6 runtime through the formal local
  media continuous path.
- Adapter/QML still do not include FFmpeg/WASAPI/P5/P6/P8 internals. QML gets
  audio/video track options from the Adapter snapshot and sends commands back
  through `PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7`.
- Continuous evidence now records real audio-track switch availability,
  command result, selected target, before/after track ids, before/after codec,
  and before/after audio frames.

Verification run locally:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
kivo_video_playback_shell.exe -ui-open artifacts/p10/ui_open_alias_visible_20260702_2121 -ui-open-close-ms 800
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_p3_|kivo_p4_|kivo_video_source_runtime_local_media_queue_tests|kivo_video_plane_|kivo_audio_plane_contract_tests|p6_|kivo_p6|kivo_p7|kivo_player_runtime_adapter_tests|kivo_p8" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/continuous_track_switch_shelter_20260702_2119 --media "<UNC Shelter 2026 directory>" --duration-ms 10000 --continuous-close-ms 500
```

Result:

- `-ui-open`: exit `0`, stderr empty, screenshot created at
  `artifacts/p10/ui_open_alias_visible_20260702_2121/ui_open_screenshot.png`.
- P3/P4/P5/P6/P7/P8/Source Runtime/Adapter focused CTest: `69/69` passed.
- Shell QML/interaction CTest: `2/2` passed.
- Real Shelter SMB continuous playback evidence:
  `artifacts/p10/continuous_track_switch_shelter_20260702_2119/continuous_playback_evidence.json`.

Real Shelter evidence fields:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `decoded_frame_count=51`
- `rendered_frame_count=51`
- `source_provider_kind=smb`
- `source_session_connected=true`
- `p3_candidate_set_created=true`
- `p3_handoff_created=true`
- `p3_handoff_consumed_by_p7=true`
- `p4_open_envelope_from_p3_created=true`
- `p4_timeline_contract_connected=true`
- `p5_local_video_runtime_connected=true`
- `p5_audio_packet_boundary_connected=true`
- `p5_audio_packets_dispatched=2613`
- `audio_decode_started=true`
- `audio_output_started=true`
- `audio_codec=truehd`
- `audio_frames_written=1119`
- `video_stream_count=4`
- `audio_stream_count=3`
- `subtitle_stream_count=5`
- `audio_track_switch_available=true`
- `audio_track_switch_command_ok=true`
- `audio_track_switch_selected=true`
- `audio_track_before_switch=2`
- `audio_track_target=3`
- `audio_track_after_switch=3`
- `audio_codec_before_switch=truehd`
- `audio_codec_after_switch=truehd`
- `playlist_queue_connected=true`
- `playlist_item_count=1`
- `p8_subtitle_connected=false`
- `errors=""`

Truthful limits:

- P0/P1 remain active governance gates. P1 is still not claimed PASS because
  the known existing ignored root-level `cmake_p5d_build.log` makes
  `verify_p1_gates.py` fail.
- P8 subtitle track inventory is discovered, but embedded PGS/subtitle packet
  ingest and rendering remain deferred. The FFmpeg PGS probe warnings in the
  Shelter run do not change this status.
- This proves real audio track switch through Adapter/P7/P5/P6 for the Shelter
  file. It does not claim bitstream passthrough, exclusive WASAPI, bit-perfect
  output, or full subtitle playback.

2026-07-03 valueless UI open and normal media launch usability update:

Implemented locally:

- `--ui-open`, `-ui-open`, `--ui-open-evidence`, and `-ui-open-evidence` now
  accept being launched without a `<dir>` value. When no directory is supplied,
  the shell writes UI-open evidence to
  `artifacts/p10/ui_open_manual`.
- Explicit output-directory forms still keep the supplied directory.
- This is a Shell argument usability fix only. It does not add a playback
  shortcut, does not bypass P7, and does not change the backend playback route.

Verification:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
kivo_video_playback_shell.exe --ui-open --ui-open-close-ms 300
kivo_video_playback_shell.exe -ui-open -ui-open-close-ms 300
kivo_video_playback_shell.exe --ui-open artifacts/p10/ui_open_explicit_dir_20260703 --ui-open-close-ms 300
kivo_video_playback_shell.exe --media "<UNC Shelter 2026 directory>"
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_player_runtime_adapter_tests" --output-on-failure
```

Result:

- No-value `--ui-open`: exit `0`, stderr empty, screenshot written to
  `artifacts/p10/ui_open_manual/ui_open_screenshot.png`.
- No-value `-ui-open`: exit `0`, stderr empty, screenshot written to
  `artifacts/p10/ui_open_manual/ui_open_screenshot.png`.
- Explicit-directory `--ui-open <dir>`: exit `0`, stderr empty, screenshot
  written to supplied directory.
- Visible no-value `--ui-open`: exit `0`, stderr empty; no `Missing value`
  system dialog.
- Normal user media launch with Shelter:
  `kivo_video_playback_shell.exe --media "<UNC Shelter 2026 directory>"`.
  The process was still running after 14 seconds and was stopped by the test
  harness; screenshot captured at
  `artifacts/p10/normal_media_visible_shelter_20260703/normal_media_visible_screen.png`
  shows a real Shelter video frame in the playback window.
- UI shell CTest: `2/2` passed.
- PlayerRuntimeAdapter CTest: `1/1` passed.
- The stderr output from normal Shelter launch contains only FFmpeg PGS subtitle
  probe warnings. It does not show a CLI parse error, Missing value error, or
  shell startup failure.

Truthful limits:

- `ui-open` remains an evidence/screenshot mode, not the normal playback entry.
  Normal playback entry is still `--media <file-or-directory>`.
- The normal `--media` screenshot proves real visible frame presentation; the
  stronger audio/video runtime proof remains the continuous playback evidence
  from `artifacts/p10/continuous_track_switch_shelter_20260702_2119`.
- P8 embedded subtitle rendering remains deferred despite discovered subtitle
  streams and PGS probe warnings.

## Resume Instructions

1. Read this file first.
2. Treat `e1394ff7d4315b502878b18a96963530213dbe6a` as the current P10
   continuous playback pushed baseline.
3. Do not reset or checkout `21881f95badce4be7c7ddb375f9c13c76286f7aa`; it is
   only the superseded first-frame baseline.
4. Continue future P10 V1 closure / real playback full closure from `e1394ff`
   unless the user gives a newer baseline.
5. Do not report new PASS claims without real current evidence for the specific
   resumed task.

2026-07-03 manual UI open and Shell surface wiring update:

Implemented locally:

- Playback page `requestOpenFile` is now connected by `PlaybackShellRoot.qml` to
  a real `QtQuick.Dialogs FileDialog`.
- Added `PlaybackShellManualMediaOpenController` in the Shell family. It
  converts the selected file URL to a native local path, calls
  `PlaybackShellRuntimeAdapterBridge.openLocalMediaFile()`, calls `play()`, and
  attaches the existing `PlaybackShellVideoSurfaceItem` to the loaded
  `PlaybackPage`.
- The manual UI open path still routes through
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter
  -> P7 PlaybackSession`; it does not call FFmpeg/WASAPI/mpv or lower P5/P6/P8
  internals from QML.
- Qt runtime deployment now scans the full `ui` family so the Shell-level
  `QtQuick.Dialogs` import is deployed with the playback shell.

Verification:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell --config Debug
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_player_runtime_adapter_tests" --output-on-failure
kivo_video_playback_shell.exe --playback-evidence artifacts/p10/manual_open_regression_20260703 --media "<UNC Shelter 2026 directory>" --playback-close-ms 300
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/manual_open_continuous_regression_20260703 --media "<UNC Shelter 2026 directory>" --duration-ms 7000 --continuous-close-ms 300
```

Result:

- UI shell CTest: `2/2` passed.
- PlayerRuntimeAdapter CTest: `1/1` passed.
- Runtime deployment log now includes `QtQuick.Dialogs`,
  `Qt6QuickDialogs2`, `Qt6QuickDialogs2QuickImpl`, and
  `Qt6QuickDialogs2Utils`.
- First-frame evidence:
  `artifacts/p10/manual_open_regression_20260703/playback_evidence.json`
  reports `open_result=accepted`, `source_opened=true`,
  `video_decode_started=true`, `first_frame_decoded=true`,
  `frame_pixels_valid=true`, `frame_rendered=true`, `adapter_video_ready=true`,
  `fake_frame_used=false`, and `errors=""`.
- Continuous evidence:
  `artifacts/p10/manual_open_continuous_regression_20260703/continuous_playback_evidence.json`
  reports `classification=PASS_CONTINUOUS_PLAYBACK`,
  `decoded_frame_count=51`, `rendered_frame_count=51`,
  `source_provider_kind=smb`, `source_session_connected=true`,
  `p3_handoff_consumed_by_p7=true`,
  `p4_timeline_contract_connected=true`,
  `p5_local_video_runtime_connected=true`,
  `p5_audio_packet_boundary_connected=true`,
  `audio_decode_started=true`, `audio_output_started=true`,
  `audio_frames_written=1280`, `audio_track_switch_command_ok=true`,
  `audio_track_switch_selected=true`, `pause_command_ok=true`,
  `resume_command_ok=true`, `seek_result=accepted`, `stop_command_ok=true`,
  and `fake_content_used=false`.
- Visual evidence screenshots at
  `artifacts/p10/manual_open_continuous_regression_20260703/playback_t5s.png`
  and `playback_t10s.png` show non-black real video frames.

Truthful limits:

- P8 embedded subtitle packet/cue/render wiring remains deferred:
  `p8_subtitle_connected=false`,
  `p8_embedded_packet_ingest_connected=false`, and
  `p8_pgs_bitmap_render_connected=false`.
- P1 is still not claimed PASS because the known pre-existing ignored
  `cmake_p5d_build.log` gate condition remains.
- The manual UI open path is now wired, but this is not a claim that the full
  playback page has reached the final Infuse-level product target.

2026-07-03 manual add, drag-drop, i18n-source cleanup, and background pump update:

Implemented locally:

- Playback Shell now exposes real manual media entry points:
  file dialog, folder dialog, and external file/folder drag-drop.
- All three entry points normalize through `PlaybackShellManualMediaOpenController`
  and then route through:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter
  -> P7 PlaybackSession`.
- Normal `--media <file-or-directory>` launch now reuses the same manual open
  controller route, so local launch evidence covers the same open/resolve/attach
  path as the user-visible manual entry.
- Playback Shell window is frameless for a more immersive player surface.
- User-visible playback QML source text in `ui/playback` and `ui/shell` was
  cleaned back to ASCII `qsTr(...)` source strings. Do not hard-code Chinese in
  QML source files; future Chinese text must come from the i18n/translation
  resource flow.
- `SettingToggleRow.qml` no longer customizes the native Qt `Switch` indicator;
  it uses a shell-local custom toggle item and no longer emits the Qt native
  style customization warning.
- `PlaybackShellVideoSurfaceItem` no longer drives playback by calling
  `pumpLocalMediaPlayback(...)` from the UI timer. It only refreshes the current
  backend frame.
- P7 `LocalMediaContinuousPlayback` now owns a background pump thread with
  mutex-protected stats/frame snapshots. P7 timeline/snapshot/clock/subtitle
  queries use `current_position_ms()` so timeline advances without the UI
  surface driving decode.

Verification:

```text
cmake --build --preset vs-debug-ffmpeg --target kivo_video_playback_shell kivo_player_runtime_adapter_tests --config Debug
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_player_runtime_adapter_tests|p7h_local_media_first_frame|p7c_command_state" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/background_pump_timeline_shelter_20260703 --media "<UNC Shelter 2026 directory>" --duration-ms 7000 --continuous-close-ms 300
kivo_video_playback_shell.exe --media "<UNC Shelter 2026 directory>"
rg -n --pcre2 "[^\\x00-\\x7F]" ui/shell ui/playback
```

Result:

- Build: passed.
- Backend focused tests: `3/3` passed.
- Shell QML/interaction tests: `2/2` passed.
- Non-ASCII source scan for `ui/shell` and `ui/playback`: no hits.
- Normal media launch through manual open controller:
  `NORMAL_MEDIA_OPEN=accepted`, `NORMAL_MEDIA_PLAY=accepted`,
  `NORMAL_MEDIA_FIRST_FRAME=yes`.
- Real Shelter SMB continuous evidence:
  `artifacts/p10/background_pump_timeline_shelter_20260703/continuous_playback_evidence.json`.
- Real local evidence:
  `artifacts/p10/background_pump_timeline_shelter_20260703/real_playback_local_evidence.json`.
- Evidence classification: `PASS_CONTINUOUS_PLAYBACK` and
  `PASS_LOCAL_REAL_PLAYBACK_VIDEO_AUDIO`.
- Evidence fields include:
  `source_provider_kind=smb`,
  `source_session_connected=true`,
  `p3_handoff_consumed_by_p7=true`,
  `p4_timeline_contract_connected=true`,
  `p5_local_video_runtime_connected=true`,
  `p5_audio_packet_boundary_connected=true`,
  `audio_decode_started=true`,
  `audio_output_started=true`,
  `audio_frames_written=1403`,
  `decoded_frame_count=55`,
  `rendered_frame_count=55`,
  `pause_holds_position=true`,
  `resume_advances_position=true`,
  `seek_result=accepted`,
  `post_seek_frame_rendered=true`,
  `audio_track_switch_command_ok=true`,
  `audio_track_switch_selected=true`,
  `fake_frame_used=false`,
  `fake_content_used=false`,
  and `errors=""`.

Truthful limits:

- The background pump removes decode pumping from the UI surface and fixes
  timeline synchronization without UI-driven pump calls. It is not a final
  hardware/GPU/HDR renderer, frame pacing, or Infuse-level performance closure.
- Shelter HEVC/TrueHD over SMB still reports only `decoded_frame_count=55` in
  this 7-second evidence run. Treat smooth playback as improved architecture,
  not final performance acceptance.
- P8 remains deferred:
  `p8_subtitle_connected=false`,
  `p8_embedded_packet_ingest_connected=false`,
  `p8_pgs_bitmap_render_connected=false`.
- The FFmpeg PGS subtitle probe warnings in the Shelter run are evidence of the
  missing real embedded subtitle ingest/render path, not a UI issue and not a
  subtitle PASS.
- P1 still cannot be claimed PASS while the known existing
  `cmake_p5d_build.log` governance failure remains.

## 2026-07-03 Update: Encoding Cleanup, P6 PCM Write Fix, And Current Performance Truth

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Playback route remains:
  `QML PlayerPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.
- UI/shell still must not call FFmpeg, WASAPI, mpv, or P5/P6/P8 internals.

Encoding / i18n correction:

- Root cause of the mojibake risk was hard-coded Chinese UI/source text without
  a guaranteed UTF-8 + translation-resource chain across QML/C++/MSVC/Windows.
- Correct source posture for this stage is ASCII source strings with `qsTr(...)`
  and later translation resources. Do not put Chinese text directly into QML or
  C++ playback source files.
- Root `CMakeLists.txt` now adds MSVC `/utf-8` globally so project and
  third-party UTF-8 headers are compiled under a stable source/execution
  charset. The previous FFmpeg `rational.h` C4819 warning did not recur after
  this change.
- Current scan over playback-related UI/backend source returned no non-ASCII
  hits:

```text
rg -n --pcre2 "[^\\x00-\\x7F]" ui/shell ui/playback backend/include/kivo/cinema_engine backend/src/decode_core backend/src/demux_core backend/src/third_party_adapter/ffmpeg_adapter backend/src/video/audio_plane/runtime/local_pcm backend/src/video/video_plane/runtime/local_media backend/src/video/playback_graph/local_media
```

P6 correction:

- The earlier P6 worker backpressure used a fixed `available_frames() < 1024`
  threshold. That was wrong because the real WASAPI shared buffer can be smaller
  than that threshold, causing audio packet starvation.
- P6 now keeps audio work inside `LocalPcmAudioRuntime` and writes each decoded
  PCM buffer in chunks according to actual `WriteResult.frames_written`.
- If WASAPI is full, only the P6 worker waits briefly and retries the remaining
  PCM bytes. P7/P5 are not blocked by UI code, and QML does not touch WASAPI.
- `audio_frames_written` now reflects actual PCM frames written, not merely the
  number of write calls.

Verification after the P6 correction:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell
```

Result:

- Debug build: passed.
- Backend focused tests: `8/8` passed.
- UI shell tests: `2/2` passed.
- Release build: passed.

Real Shelter SMB evidence after the P6 correction:

```text
artifacts/p10/p6_pcm_complete_write_shelter_debug_20260703/continuous_playback_evidence.json
artifacts/p10/p6_pcm_complete_write_shelter_release_20260703/continuous_playback_evidence.json
```

Debug evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `playback_duration_observed_ms=9202`
- `decoded_frame_count=79`
- `rendered_frame_count=79`
- `audio_frames_decoded=1901`
- `audio_frames_written=121600`
- `p5_audio_packets_dispatched=4011`
- `pause_holds_position=true`
- `resume_advances_position=true`
- `seek_result=accepted`
- `post_seek_frame_rendered=true`
- `shell_core_playback_loop=false`
- `p8_subtitle_connected=false`

Release evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `playback_duration_observed_ms=8769`
- `decoded_frame_count=87`
- `rendered_frame_count=87`
- `audio_frames_decoded=1758`
- `audio_frames_written=112480`
- `p5_audio_packets_dispatched=4531`
- `pause_holds_position=true`
- `resume_advances_position=true`
- `seek_result=accepted`
- `post_seek_frame_rendered=true`
- `shell_core_playback_loop=false`
- `p8_subtitle_connected=false`

Current truthful limits:

- This is real playback evidence, not fake content and not mpv/local-player
  workaround evidence.
- Performance is still not Infuse-class. The current continuous video route is
  P2 custom IO -> P5 FFmpeg demux -> software HEVC decode -> RGBA -> Qt image
  surface. The formal P5 D3D11/upload/presenter contract family exists, but the
  P10 continuous playback route is not yet connected to a continuous hardware
  decode / hardware presenter path.
- TrueHD decoding produced many FFmpeg warnings during the Shelter run while
  structured evidence still reported `audio_error=""` and PCM frames were
  written. Treat this as a P5/P6 audio-quality risk to investigate, not as a
  full audio-quality PASS.
- P8 remains deferred. Subtitle track discovery exists, but embedded subtitle
  packet ingest, PGS bitmap render, and subtitle frame handoff are not connected.
- P7/P4/P2/P3/P5/P6 route is real enough for local continuous proof, but not a
  final premium-player closure until the P5 hardware presenter and P8 embedded
  subtitle paths are connected and validated.
- A short FFmpeg video decoder thread-count experiment was tried in
  `FfmpegAdapter::create_codec_context()` and rejected. Release evidence
  `artifacts/p10/release_video_decoder_threads_shelter_20260703/continuous_playback_evidence.json`
  produced `decoded_frame_count=108` but stretched
  `playback_duration_observed_ms=11872`, so effective throughput did not improve.
  The thread-count code was removed; do not reintroduce it without a better P5
  decode/presenter design and evidence.

## 2026-07-03 Update: Packet Side Data, Decoder Flush Semantics, And Remaining Audio/Perf Risk

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Playback route remains:
  `QML PlayerPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.
- UI still does not call FFmpeg, WASAPI, mpv, or P5/P6/P8 internals.

Backend corrections:

- FFmpeg packet side data is now preserved from demux read through
  `KivoPacket` and into `RealSoftwareDecodeRuntime::decode(...)` /
  `decode_video_rgba(...)`.
- Audio packets dispatched from P5 to P6 now carry packet side data through
  `LocalVideoAudioPacket` and `LocalPcmAudioPacket`.
- `FfmpegAdapter::flush_decoder(...)` now uses `avcodec_flush_buffers(...)`
  for seek/reset semantics. The previous implementation sent a null packet and
  drained, which is EOF/drain behavior and is wrong for seek reset.

Verification after side-data and flush correction:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell -- /m:1 /v:minimal
```

Result:

- Debug build: passed.
- Backend focused tests: `8/8` passed.
- UI shell tests: `2/2` passed.
- Release build: passed.
- Playback-source non-ASCII scans:
  `ui/shell`, `ui/playback`, and the touched backend wiring files returned no
  matches.

Real Shelter SMB evidence:

```text
artifacts/p10/side_data_audio_shelter_release_20260703/continuous_playback_evidence.json
artifacts/p10/ffmpeg_flush_buffers_shelter_release_20260703/continuous_playback_evidence.json
```

Side-data evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `playback_duration_observed_ms=8638`
- `decoded_frame_count=95`
- `rendered_frame_count=95`
- `audio_frames_decoded=1616`
- `audio_frames_written=103360`
- `p5_audio_packets_dispatched=4771`
- `shell_core_playback_loop=false`
- `p8_subtitle_connected=false`
- TrueHD warning lines in captured stderr: `1909`

Flush-buffers evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `playback_duration_observed_ms=8793`
- `decoded_frame_count=95`
- `rendered_frame_count=95`
- `audio_frames_decoded=1646`
- `audio_frames_written=105280`
- `p5_audio_packets_dispatched=4771`
- `pause_command_ok=true`
- `pause_holds_position=true`
- `resume_command_ok=true`
- `resume_advances_position=true`
- `seek_result=accepted`
- `post_seek_frame_rendered=true`
- `shell_core_playback_loop=false`
- `p8_subtitle_connected=false`
- TrueHD warning lines in captured stderr: `1765`

Current truthful limits:

- Packet side data and seek flush semantics are now corrected at the backend
  boundary, but the Shelter TrueHD run still emits many FFmpeg TrueHD warnings.
  Treat audio quality as still under investigation, not fully accepted.
- The current Release evidence still renders only about 95 frames over roughly
  8.8 seconds for the Shelter HEVC 10-bit source. This is real video playback,
  but it is not premium smooth playback.
- The dominant performance gap remains the P5 continuous video path:
  software HEVC decode -> RGBA conversion -> Qt image surface. The existing P5
  D3D11/presenter contracts are not yet connected to the continuous playback
  route as a real hardware presenter.
- P8 remains deferred: subtitle tracks are discovered, but embedded subtitle
  packet ingest, PGS bitmap render, and subtitle frame handoff are not connected.
- Do not report Infuse-level completion until P5 hardware presentation, audio
  quality, and P8 subtitle playback are actually connected and verified.

## 2026-07-03 Update: Normal Media Entry Single Surface Check

Correction:

- Normal `--media <path>` launch already routes through
  `PlaybackShellManualMediaOpenController`, which opens through
  `PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7`.
- `schedule_normal_media_launch(...)` was attaching a second
  `PlaybackShellVideoSurfaceItem` after the manual controller had already
  attached one. That redundant surface was removed.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg/backend -C Debug -R "kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg/ui -C Debug -R "kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test" --output-on-failure
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell -- /m:1 /v:minimal
```

Result:

- Debug build: passed.
- Backend focused tests: `3/3` passed.
- UI shell tests: `2/2` passed.
- Release build: passed.
- Normal Release UI launch with quoted Shelter UNC path:
  `artifacts/p10/normal_media_single_surface_release_quoted_20260703/stderr.txt`.
- Normal launch status:
  `NORMAL_MEDIA_OPEN=accepted`,
  `NORMAL_MEDIA_PLAY=accepted`,
  `NORMAL_MEDIA_FIRST_FRAME=yes`.
- No residual `kivo_video_playback_shell` process remained after the launch
  verification was stopped.

Note:

- The unquoted `Start-Process -ArgumentList @('--media', $media)` harness form
  splits the UNC path at spaces and is not a valid reproduction of user launch
  behavior. Quote the media argument when launching paths with spaces.
- This removes one UI-side duplicate paint/refresh cost. It is not the final
  performance fix. The real smooth-playback closure still requires the P5
  hardware presenter/frame pacing path.

## 2026-07-03 Update: Shared Frame Snapshot and FFmpeg Video Threading

Corrections kept in current local state:

- P5/P7/UI frame payload now passes an immutable shared RGBA frame reference
  instead of copying the full RGBA vector through P5 -> P7 query -> Qt bridge.
- `PlaybackShellRuntimeAdapterBridge::pullVideoFrameImage()` now builds the
  `QImage` over the shared frame memory with a cleanup holder, removing the
  per-frame row memcpy in the shell bridge.
- FFmpeg video decoder contexts now enable video-only frame/slice threading at
  the FFmpeg adapter boundary. Audio decoder contexts are not changed.
- The failed decoder-drain experiment was removed because it increased the
  observed evidence runtime and warning volume without enough frame benefit.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell -- /m:1 /v:minimal
```

Result:

- Debug build: passed.
- Focused backend/UI tests: `10/10` passed.
- Release build: passed.
- Final Shelter SMB evidence:
  `artifacts/p10/threaded_shared_frame_final_shelter_release_20260703/continuous_playback_evidence.json`.

Final Shelter evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `source_provider_kind=smb`
- `video_codec=hevc`
- `audio_codec=truehd`
- `playback_duration_observed_ms=8601`
- `decoded_frame_count=123`
- `rendered_frame_count=123`
- `audio_frames_decoded=1143`
- `audio_frames_written=73120`
- `p5_audio_packets_dispatched=6595`
- `source_session_connected=true`
- `p3_handoff_consumed_by_p7=true`
- `p4_timeline_contract_connected=true`
- `p5_local_video_runtime_connected=true`
- `audio_output_started=true`
- `p8_subtitle_connected=false`
- `fake_frame_used=false`
- `errors=""`
- TrueHD warning lines in captured stderr: `2130`

Comparison against the previous software path:

- `ffmpeg_flush_buffers_shelter_release_20260703`: `95` rendered frames over
  `8793ms`.
- `shared_rgba_frame_shelter_release_20260703`: `95` rendered frames over
  `8645ms`.
- `threaded_shared_frame_final_shelter_release_20260703`: `123` rendered
  frames over `8601ms`.

Encoding note:

- The evidence JSON is UTF-8 and contains the real UNC path correctly. Windows
  PowerShell default `Get-Content` can display a UTF-8 JSON without BOM as
  mojibake. Use explicit UTF-8 reads, for example
  `[System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)`.
- Source/QML touched in this pass should keep user-visible text as ASCII
  `qsTr(...)` keys until the i18n translation resource chain is in place.

Current truthful limits:

- This is still not Infuse-level playback. It is real P2/P3/P4/P5/P6/P7 wired
  playback with a better software path, but still below mature-player smoothness.
- The biggest remaining video gap is still P5 continuous GPU presentation:
  current continuous playback is software HEVC decode -> CPU RGBA conversion ->
  Qt painted surface. Existing D3D11 upload/presenter code is only first-frame
  proof or fake/contract service, not a real continuous presenter.
- TrueHD still emits warnings and audio quality remains under investigation.
- P8 remains deferred: subtitle tracks are discovered, but embedded subtitle
  packet ingest and subtitle frame handoff are not connected.

## 2026-07-03 Update: TrueHD Fail-Closed and DTS Default Audio Fallback

Problem fixed in the local checkout:

- `kivo_real_demux_decode_integration_test.exe` no longer aborts when the
  `KIVO_SAMPLE_H264_AAC_MP4` variable points at a non-MP4/TrueHD MKV. The test
  now keeps its original scope: MP4/M4V/MOV H.264 + AAC. Other inputs are
  reported as `SKIP` before deep FFmpeg probe/decode.
- `RealSoftwareDecodeRuntime::decode()` and `decode_video_rgba()` now honor the
  FFmpeg `avcodec_send_packet` EAGAIN contract by draining frames before
  retrying the same packet, instead of treating EAGAIN as a request for more
  input.
- FFmpeg audio frame extraction now copies planar audio by real per-channel
  sample bytes. It no longer depends on `frame->linesize[ch]` for every channel,
  which can be zero for channels after the first plane.
- P6 local PCM audio now fail-closes `truehd`/`mlp` until real passthrough or a
  formal compatible-fallback policy is complete. It does not feed TrueHD into
  PCM decode and let FFmpeg spam decoder warnings.
- P7 local media track inventory now defaults to the first real audio stream
  that does not require passthrough/fallback. For the Shelter SMB sample this
  selects the real `dts` track by default instead of the TrueHD track.
- The continuous-playback evidence script restores the previous working audio
  track if its track-switch probe lands on an unsupported track. It still records
  the switch attempt fields, but the final playback snapshot is the recovered
  usable state.
- `kivo_cinema_engine` now links `swscale` at the FFmpeg boundary because
  `ffmpeg_adapter.cpp` owns `receive_frame_rgba()`.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_real_demux_decode_integration_test -- /m:1 /v:minimal
KIVO_SAMPLE_H264_AAC_MP4=C:\kivo video\tests\real\kivo_h264_aac_35s_golden.mp4 kivo_real_demux_decode_integration_test.exe
KIVO_SAMPLE_H264_AAC_MP4=\\192.168.5.1\...\Shelter ... .mkv kivo_real_demux_decode_integration_test.exe
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
```

Result:

- Debug test target build: passed.
- MP4 H.264/AAC direct integration test: passed.
- Shelter MKV passed to the MP4/AAC test: `SKIP` before deep decode, no Debug
  Runtime abort and no hang.
- Focused CTest: `12/12` passed.
- Release playback shell build: passed.
- Final Shelter SMB evidence:
  `artifacts/p10/dts_default_restore_truehd_shelter_release_20260703/continuous_playback_evidence.json`.

Final Shelter evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `source_provider_kind=smb`
- `video_codec=hevc`
- `audio_codec=dts`
- `audio_output_started=true`
- `audio_frames_decoded=145`
- `audio_frames_written=74016`
- `audio_error=""`
- `decoded_frame_count=123`
- `rendered_frame_count=123`
- `p5_audio_packets_dispatched=517`
- `p3_handoff_consumed_by_p7=true`
- `p4_timeline_contract_connected=true`
- `p5_local_video_runtime_connected=true`
- `p8_subtitle_connected=false`
- `errors=""`
- TrueHD warning/error lines in captured stderr: `0`
- Remaining PGS/probe warning lines in captured stderr: `54`

Current truthful limits:

- TrueHD passthrough is still not implemented. TrueHD is fail-closed and the
  real DTS fallback is used for this Shelter file.
- Manual selection of a TrueHD track is not a complete audio playback path yet;
  it must wait for P6 passthrough/fallback policy closure.
- PGS subtitle probe warnings remain. P8 embedded packet ingest, PGS bitmap
  render, and subtitle frame handoff are still deferred.
- This does not close the larger Infuse-level goal. P5 continuous GPU
  presentation/frame pacing is still the biggest playback-quality gap.

## 2026-07-04 Update: Hidden-Control UI Polish, Ready Track Preselection, And P8 Embedded SubRip Ingest

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI work followed the current product direction: Infuse-like quiet playback
  surface, visible beauty first, deeper capabilities hidden behind a small More
  hub and settings sheets.
- Backend route remains:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.
- QML still does not call P4/P5/P6/P8 internals.

Frontend corrections:

- `MorePlaybackSheet.qml` is now a hidden hub instead of a raw action dump.
  Subtitles, Audio, Episodes, Versions, and Settings are reachable from More,
  while the main overlay remains quiet.
- `PlaybackOverlay.qml` opens `MorePlaybackSheet` for `activeSheet == "more"`
  and routes Settings to a separate `SettingsSheet`.
- `PlaybackPage.qml` routes settings actions to the settings sheet and keeps
  playback commands behind the adapter bridge.
- `PlaybackIconSet.qml` gained subtitle, versions, settings, chevron, plus, and
  minus icons for the hidden hub and stepper controls.
- `SettingsSheet.qml` uses ASCII `qsTr(...)` source strings and the stepper rows
  use icon buttons instead of text `+` / `-` buttons.

Backend/P7/P8 corrections:

- P7 now accepts explicit audio/video/subtitle track preselection in `Ready`
  state without forcing playback to `Playing`. Invalid track ids still reject
  with `InvalidCommand`.
- `cycle_audio_track()` and `cycle_subtitle_track()` restore the previous
  semantic state: `Ready` stays `Ready`, `Paused` stays `Paused`, and `Playing`
  returns to `Playing`.
- The continuous evidence harness preselects the first subtitle track after
  open and before play only for evidence. This proves the embedded subtitle
  packet boundary without changing product default subtitle behavior.
- P8 embedded packet ingest is connected for the text/SubRip-like packet path.
  PGS bitmap rendering remains explicitly not connected.

Verification after this update:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_real_demux_decode_integration_test|kivo_video_source_runtime_local_media_queue_tests)" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/p8_embedded_subrip_ingest_20260704 --media artifacts/p10/p8_embedded_subrip_ingest_20260704/h264_aac_subrip.mkv --duration-ms 6500 --continuous-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Focused CTest: `6/6` passed.
- Media-chain CTest: `12/12` passed.
- Rebuilt UI QML load/interaction tests after QML resource changes: `2/2`
  passed.
- P8 evidence rerun:
  `artifacts/p10/p8_embedded_subrip_ingest_20260704/continuous_playback_evidence.json`.
- No residual `kivo*` process remained after the P8 evidence run.
- Playback-source non-ASCII scan over touched UI/backend/test files returned no
  matches.

P8 evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `container=matroska,webm`
- `video_codec=h264`
- `audio_codec=aac`
- `decoded_frame_count=1459`
- `rendered_frame_count=1459`
- `p5_subtitle_packet_boundary_connected=true`
- `p5_subtitle_packets_dispatched=2`
- `p8_embedded_packet_ingest_connected=true`
- `p8_subtitle_packets_submitted=2`
- `p8_pgs_bitmap_render_connected=false`
- `p8_subtitle_connected=false`
- `p8_unsupported_feature_count=0`
- `subtitle_status=partial_embedded_packet_ingest_no_bitmap_render`
- `fake_frame_used=false`
- `fake_content_used=false`
- `errors=""`

Current truthful limits:

- This update does not close full P8 subtitle rendering. It proves embedded
  packet ingest for the text/SubRip-like path only.
- PGS bitmap render, subtitle composition, styled subtitle layout, and final
  subtitle frame handoff are still deferred.
- TrueHD passthrough remains unimplemented. The correct current behavior is
  fail-closed plus real DTS fallback where available.
- The largest Infuse-level playback gap remains P5 continuous hardware
  presentation/frame pacing. The current continuous route is still not a final
  premium GPU presenter path.

## 2026-07-04 Update: P8 Text Cue Ring and Infuse-Style Surface Tightening

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Backend route remains:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.
- QML still does not call P4/P5/P6/P8 internals.

Backend/P8 correction:

- P8 embedded text cue storage now keeps a small cue ring instead of replacing
  the only active cue with the latest packet.
- Frame lease selection now chooses the text cue matching playback time and
  keeps the selected cue slot behind the P8 access token.
- Text buffer access copies the cue selected by the frame lease. A later cue no
  longer hides an earlier active cue.
- PGS/bitmap policy remains fail-closed/deferred. Full subtitle composition is
  still not claimed.

Frontend tightening:

- Main playback chrome was tightened toward the product direction: visible
  surface first, controls quiet, deeper capability behind More.
- The top-left playback action now uses a back chevron instead of an X-like
  close glyph.
- The bottom tool row no longer exposes a separate subtitles button. Subtitles,
  audio, versions, episodes, and settings remain reachable through More.
- The subtitle sheet no longer shows size/position/delay action buttons; tuning
  stays in the settings sheet.
- The versions sheet no longer exposes technical detail rows in the user-facing
  panel.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_p8d_source_privacy_tests kivo_p8e_frame_lease_tests kivo_p8f_text_font_privacy_tests kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_p7c_command_state_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell -- /m:1 /v:minimal
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_polish_hidden_controls_after_20260704 --interaction-close-ms 500
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/p8_embedded_subrip_text_frame_ring_20260704 --media artifacts/p10/p8_embedded_subrip_ingest_20260704/h264_aac_subrip.mkv --duration-ms 6500 --continuous-close-ms 500
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/p8_embedded_subrip_text_frame_ring_release_20260704 --media artifacts/p10/p8_embedded_subrip_ingest_20260704/h264_aac_subrip.mkv --duration-ms 6500 --continuous-close-ms 500
```

Result:

- Debug P8/shell build: passed.
- Focused P8/P7/Adapter/UI CTest: `7/7` passed.
- Expanded media-chain/P8/UI CTest after UI changes: `15/15` passed.
- Release playback shell build: passed.
- UI interaction evidence:
  `artifacts/p10/ui_polish_hidden_controls_after_20260704/interaction_evidence.json`.
- Debug P8 text-frame evidence:
  `artifacts/p10/p8_embedded_subrip_text_frame_ring_20260704/continuous_playback_evidence.json`.
- Release P8 text-frame evidence:
  `artifacts/p10/p8_embedded_subrip_text_frame_ring_release_20260704/continuous_playback_evidence.json`.
- Touched source non-ASCII scan returned no matches.
- No residual `kivo*` process remained after evidence runs.

Release P8 evidence summary:

- `classification=PASS_CONTINUOUS_PLAYBACK`
- `decoded_frame_count=1534`
- `rendered_frame_count=1534`
- `p8_embedded_packet_ingest_connected=true`
- `p8_subtitle_packets_submitted=2`
- `p8_text_frame_runtime_connected=true`
- `p8_text_frame_available_t5=true`
- `p8_text_frame_primary_t5=real embedded subtitle packet`
- `p8_text_frame_connected=true`
- `p8_subtitle_connected=true`
- `p8_full_subtitle_render_connected=false`
- `p8_pgs_bitmap_render_connected=false`
- `subtitle_status=partial_embedded_text_frame_no_bitmap_render`
- `fake_frame_used=false`
- `fake_content_used=false`
- `errors=""`

Current truthful limits:

- P8 is now connected for embedded SubRip-like packet ingest plus a time-matched
  text frame through the runtime path. It is still not full styled subtitle
  rendering.
- PGS bitmap render, subtitle composition, styling/layout fidelity, and final
  mature subtitle renderer behavior remain deferred.
- TrueHD passthrough remains unimplemented; current correct behavior is
  fail-closed plus real DTS fallback where available.
- The largest Infuse-level playback gap remains P5 continuous hardware
  presentation/frame pacing.

## 2026-07-04 Update: Quiet Keyboard Chrome Policy

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI-only polish inside the existing playback input family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- Keyboard transport and level actions no longer force the full playback chrome
  visible.
- `Space`, left/right seek, up/down volume, and mute remain wired to the runtime
  adapter route, but do not wake the whole overlay just because the user made a
  small playback adjustment.
- Sheet shortcuts still reveal chrome because they intentionally open hidden
  capability panels.
- This keeps the visible surface closer to the current product direction:
  content first, quiet controls, deeper power behind More/sheets.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_quiet_keyboard_chrome_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- UI interaction evidence:
  `artifacts/p10/ui_quiet_keyboard_chrome_20260704/interaction_evidence.json`.
- UI screenshot:
  `artifacts/p10/ui_quiet_keyboard_chrome_20260704/interaction_screenshot.png`.
- Evidence summary: `qml_loaded=true`, `screenshot_exists=true`,
  `qml_state_updated=true`, `fake_media_used=false`,
  `fake_content_used=false`, `typed_p7_harness=true`,
  `residual_process=false`, `exit=normal`.
- Touched QML non-ASCII scan returned no matches.
- No residual `kivo*` process remained after the evidence run.

Current truthful limits:

- This is UI chrome behavior polish, not a P5 smoothness closure.
- It does not implement the missing P5 continuous hardware presenter/frame
  pacing path.
- It does not change the current P8 limit: embedded text frame partial path is
  connected, but full subtitle render and PGS bitmap render remain deferred.

## 2026-07-04 Update: Scrollable Playback Sheets

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI-only polish inside the existing playback sheet family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `PlaybackSheet.qml` now supports long content through an internal `Flickable`
  instead of clipping rows when a media file exposes many tracks or settings.
- Sheet height is still capped to keep the video surface dominant.
- The sheet keeps the same default content slot used by subtitle/audio/episode/
  version/settings sheets, so no feature sheet had to learn a new ownership
  pattern.
- Open/close now has a subtle vertical motion paired with opacity, keeping the
  panel feeling lighter without exposing more controls on the main surface.
- A minimal scroll indicator appears only while the sheet content is moving.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_scrollable_playback_sheet_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- UI interaction evidence:
  `artifacts/p10/ui_scrollable_playback_sheet_20260704/interaction_evidence.json`.
- Evidence summary: `qml_loaded=true`, `screenshot_exists=true`,
  `qml_state_updated=true`, `fake_media_used=false`,
  `fake_content_used=false`, `typed_p7_harness=true`,
  `residual_process=false`, `exit=normal`.
- Touched QML non-ASCII scan returned no matches.
- No residual `kivo*` process remained after the evidence run.

Current truthful limits:

- This improves hidden-control usability. It is not a P5 smoothness closure.
- It does not implement full P8 styled subtitle rendering or PGS bitmap render.
- TrueHD passthrough remains unimplemented.

## 2026-07-04 Update: Subtitle Text Scale Reaches Visible Overlay

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI/evidence polish only; no direct QML call to P4/P5/P6/P8 internals.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `SubtitleOverlay.qml` now consumes the existing `subtitleSize` setting through
  a bounded `textScale`.
- Primary and secondary subtitle font sizes scale from the shared playback
  typography values instead of ignoring the setting.
- `PlaybackOverlay.qml` passes the page-level subtitle size into the visible
  subtitle overlay.
- The hidden Settings sheet already exposed `Text size`; this update makes that
  deep setting actually affect user-visible subtitle rendering.

Evidence correction:

- Interaction evidence now exercises `setSubtitleSize(1.18)`.
- `playback_shell_interaction_evidence.cpp` records `subtitleSize` in
  `qml_state_before` and `qml_state_after`.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_subtitle_text_scale_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- UI interaction evidence:
  `artifacts/p10/ui_subtitle_text_scale_20260704/interaction_evidence.json`.
- Evidence summary: `qml_loaded=true`, `screenshot_exists=true`,
  `qml_state_updated=true`, `setSubtitleSize=accepted`,
  `qml_state_before.subtitleSize=1`, `qml_state_after.subtitleSize=1.18`,
  `fake_media_used=false`, `fake_content_used=false`,
  `typed_p7_harness=true`, `residual_process=false`, `exit=normal`.
- Touched source non-ASCII scan returned no matches.
- No residual `kivo*` process remained after the evidence run.

Current truthful limits:

- This connects the existing text-size user setting to visible QML subtitle
  text. It is not full P8 styled subtitle rendering.
- PGS bitmap render and mature subtitle composition remain deferred.
- P5 continuous hardware presenter/frame pacing remains the largest playback
  smoothness gap.

## 2026-07-04 Update: Hide Immature Frame and Tone Controls

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI-only polish inside the existing playback sheet family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `SettingsSheet.qml` no longer exposes the user-facing `Frame` aspect segment
  or `Tone` mapping segment.
- The commands and snapshot state remain wired through the bridge/Adapter/P7
  policy path for evidence and future presenter work, but they are hidden from
  the product surface until P5 has a mature visible presenter/HDR/tone mapping
  closure.
- The Settings sheet now stays focused on controls the current playback surface
  can honestly reflect: playback speed, text size, subtitle sync, audio sync,
  and continue-playing preference.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_hide_unmatured_frame_tone_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- UI interaction evidence:
  `artifacts/p10/ui_hide_unmatured_frame_tone_20260704/interaction_evidence.json`.
- Evidence summary: `qml_loaded=true`, `screenshot_exists=true`,
  `qml_state_updated=true`, `fake_media_used=false`,
  `fake_content_used=false`, `typed_p7_harness=true`,
  `residual_process=false`, `exit=normal`.
- Deep policy commands still route through the bridge for evidence:
  `setAspectMode=accepted`, `setToneMappingMode=accepted`.
- Touched QML non-ASCII scan returned no matches.
- No residual `kivo*`, `ctest*`, `cmake*`, or `MSBuild*` process remained
  after the evidence run.

Current truthful limits:

- This is product-surface honesty, not P5 presenter closure.
- Aspect and tone policy state must not be marketed as mature visible playback
  behavior until the video presenter actually consumes those policies.
- P5 continuous hardware presenter/frame pacing remains the largest
  Infuse-level quality gap.

## 2026-07-04 Update: Context Menu Follows Available Actions

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI-only polish inside the existing playback menu/page family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `PlaybackContextMenu.qml` now takes the same availability booleans as the
  visible/More playback surface.
- Subtitles, Audio, Episodes, and Versions are hidden from the right-click menu
  unless the current snapshot actually exposes those options.
- The context-menu label `Audio Tracks` was tightened to `Audio`, matching the
  quieter product language used by the hidden More hub.
- `PlaybackPage.qml` passes real option availability from subtitle/audio/
  episode/version lists into the context menu.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_context_menu_available_actions_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- UI interaction evidence:
  `artifacts/p10/ui_context_menu_available_actions_20260704/interaction_evidence.json`.
- Evidence summary: `qml_loaded=true`, `screenshot_exists=true`,
  `qml_state_updated=true`, `fake_media_used=false`,
  `fake_content_used=false`, `typed_p7_harness=true`,
  `residual_process=false`, `exit=normal`.
- Touched QML non-ASCII scan returned no matches.
- No residual `kivo*`, `ctest*`, `cmake*`, or `MSBuild*` process remained
  after the evidence run.

Current truthful limits:

- This closes a product-surface leak only. It does not change P5 presenter
  quality, TrueHD support, or full P8 subtitle-render status.

## 2026-07-04 Update: Truthful Dispatch and Quiet Track Labels

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI/shell polish inside the existing playback input, sheet, overlay, and
  snapshot mapping families.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend and shell correction:

- `PlaybackAdapterDispatch.qml` now respects the boolean returned by bridge
  calls. A command rejected by P7/Adapter no longer looks successful in the QML
  dispatch path merely because the method exists.
- `PlaybackShellRuntimeAdapterBridge` exposes `disableSubtitle()` through the
  same Adapter/P7 route as the rest of playback control.
- `SubtitleSheet.qml` now has a quiet `Off` row before subtitle choices.
  Selecting it calls `disableSubtitle` rather than trying to fake a subtitle
  track id.
- `AudioTrackSheet.qml` title and hidden audio tooltip were tightened to
  `Audio`.
- Default track labels from `playback_shell_snapshot_mapper.cpp` now use
  `Audio 1`, `Subtitle 1`, and `Video 1` instead of the engineering-style
  `Audio track #1` / `Subtitle track #1` / `Video track #1`.
- The interaction evidence harness now records both `selectSubtitleTrack` and
  `disableSubtitle`.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_subtitle_off_truthful_dispatch_labels_final_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- UI interaction evidence:
  `artifacts/p10/ui_subtitle_off_truthful_dispatch_labels_final_20260704/interaction_evidence.json`.
- Evidence summary: `qml_loaded=true`, `screenshot_exists=true`,
  `qml_state_updated=true`, `fake_media_used=false`,
  `fake_content_used=false`, `typed_p7_harness=true`,
  `residual_process=false`, `exit=normal`.
- Command evidence:
  `selectSubtitleTrack=accepted`, `disableSubtitle=accepted`,
  rejected commands remain reported as `rejected`.
- Snapshot evidence before close:
  `audio_labels=Audio 1, Audio 2, Audio 3`,
  `subtitle_labels=Subtitle 1, Subtitle 2, Subtitle 3, Subtitle 4`,
  `selectedSubtitleTrack=0`, `subtitleEnabled=false`.
- Touched source non-ASCII scan returned no matches.
- No `Audio track #`, `Subtitle track #`, `Video track #`, or `Audio Tracks`
  strings remain in `ui/playback` or `ui/shell` source.
- No residual `kivo*`, `ctest*`, `cmake*`, or `MSBuild*` process remained
  after the evidence run.

Current truthful limits:

- This improves truthful UI control and product language. It does not implement
  full P8 styled subtitle rendering or PGS bitmap rendering.
- P5 continuous hardware presenter/frame pacing remains the largest
  Infuse-level playback-quality gap.
- TrueHD passthrough remains unimplemented; current correct behavior remains
  fail-closed plus real fallback where available.

## 2026-07-04 Update: Video Surface Underlay Layering

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI-only layering polish inside the existing playback page family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `PlaybackPage.qml` now makes the stacking order explicit:
  placeholder/video base at `z=0`, dynamic real video surface at C++ `z=1`,
  mouse interaction at `z=5`, and playback overlay/chrome at `z=10`.
- This prevents `PlaybackShellVideoSurfaceItem` from covering the Infuse-style
  overlay, gradients, transport controls, and sheet interaction after a real
  media file is opened.
- `showPlaceholderVideo=false` still hides the placeholder once the real video
  surface is attached; this change only fixes visual/interaction layering.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
kivo_video_playback_shell.exe --playback-evidence artifacts/p10/ui_video_surface_underlay_20260704 --media artifacts/p10/p8_embedded_subrip_ingest_20260704/h264_aac_subrip.mkv --playback-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `15/15` passed.
- Real local playback evidence:
  `artifacts/p10/ui_video_surface_underlay_20260704/playback_evidence.json`.
- Screenshot:
  `artifacts/p10/ui_video_surface_underlay_20260704/playback_screenshot.png`.
- Evidence summary: `open_result=accepted`, `first_frame_decoded=true`,
  `frame_rendered=true`, `frame_pixels_valid=true`,
  `adapter_video_ready=true`, `qml_visible=true`,
  `fake_frame_used=false`, `fake_content_used=false`, `errors=""`,
  `exit=normal`, `residual_process=false`.
- Visual inspection confirmed real video is under the playback overlay and the
  chrome remains visible over the frame.
- Touched QML non-ASCII scan returned no matches.
- No residual `kivo*`, `ctest*`, `cmake*`, or `MSBuild*` process remained
  after the evidence run.

Current truthful limits:

- This fixes surface/chrome layering only. It is not P5 continuous hardware
  presenter/frame pacing closure.
- Full P8 styled subtitle rendering, PGS bitmap rendering, and TrueHD
  passthrough remain unfinished.

## 2026-07-04 Update: Video Surface Aspect Geometry

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI/shell surface polish inside the existing playback shell family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `PlaybackShellVideoSurfaceItem` no longer stretches every frame to the full
  window rectangle.
- The Qt-painted video surface now fills the item black first, then draws the
  decoded frame using the current adapter aspect policy:
  `fit`, `fill`, `stretch`, or `original`.
- Default `fit` preserves source aspect ratio and centers the frame, so
  portrait or non-16:9 media gets letterbox/pillarbox instead of distortion.
- `fill` preserves aspect ratio and crops overflow inside the surface clip.
- `stretch` keeps the old full-rect behavior for the hidden policy path.
- `original` draws 1:1 centered when the source fits; if the source is larger
  than the surface, it falls back to fit.
- Aspect geometry was split into
  `ui/shell/playback_shell_video_surface_geometry.*` so the surface item keeps
  painting responsibility and the geometry is directly testable.
- `PlaybackShellVideoSurfaceItem` refreshes aspect policy from bridge snapshots
  and repaints when `snapshotChanged` is emitted, so hidden aspect commands are
  still wired without exposing a visible Frame control.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell kivo_playback_shell_video_surface_geometry_test -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
kivo_video_playback_shell.exe --playback-evidence artifacts/p10/ui_aspect_fit_surface_real_sample_20260704 --media artifacts/p10/p8_embedded_subrip_ingest_20260704/h264_aac_subrip.mkv --playback-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest including the new aspect geometry test:
  `16/16` passed.
- Real local playback evidence:
  `artifacts/p10/ui_aspect_fit_surface_real_sample_20260704/playback_evidence.json`.
- Screenshot:
  `artifacts/p10/ui_aspect_fit_surface_real_sample_20260704/playback_screenshot.png`.
- Evidence summary: `open_result=accepted`, `source_opened=true`,
  `first_frame_decoded=true`, `frame_rendered=true`,
  `frame_pixels_valid=true`, `adapter_video_ready=true`,
  `qml_visible=true`, `fake_frame_used=false`, `fake_content_used=false`,
  `errors=""`, `exit=normal`, `residual_process=false`.
- Generated portrait H264/AAC MP4 samples under
  `artifacts/p10/ui_aspect_fit_surface_20260704/` verified through
  `kivo_real_demux_decode_integration_test` as real `360x640` H264/AAC media.
- Direct shell `--playback-evidence` with those portrait MP4 samples hung before
  writing evidence. The single shell process was stopped and no residual process
  remained. Treat this as a separate evidence-launch edge to investigate, not as
  proof against the aspect geometry fix.
- Touched source non-ASCII scan returned no matches.
- No residual `kivo*`, `ctest*`, `cmake*`, or `MSBuild*` process remained
  after verification.

Current truthful limits:

- This makes the current Qt-painted software video surface respect aspect
  policy. It is not P5 continuous hardware presenter/frame pacing closure.
- P5 remains software decode to RGBA/Qt-painted presentation for the current
  continuous route.
- Full P8 styled subtitle rendering, PGS bitmap rendering, and TrueHD
  passthrough remain unfinished.

## 2026-07-04 Update: Settings Subtitle Controls Follow Availability

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI-only visibility polish inside the existing playback overlay/settings
  family.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Frontend correction:

- `SettingsSheet.qml` no longer always shows subtitle text size and subtitle
  sync controls.
- `PlaybackOverlay.qml` now computes `subtitleControlsAvailable` from existing
  subtitle options or active subtitle text and passes it to Settings.
- When a media/session has no available subtitle surface, Settings keeps only
  the general controls such as Speed, Audio sync, and Continue playing.
- The underlying commands remain wired through QML -> bridge -> Adapter/P7.
  This is presentation discipline only: do not show controls that have no
  current user meaning.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
kivo_video_playback_shell.exe --interaction-evidence artifacts/p10/ui_settings_subtitle_controls_visibility_20260704 --interaction-close-ms 500
```

Result:

- Debug playback shell build: passed.
- Expanded media-chain/P8/UI CTest: `16/16` passed.
- UI interaction evidence:
  `artifacts/p10/ui_settings_subtitle_controls_visibility_20260704/interaction_evidence.json`.
- Screenshot:
  `artifacts/p10/ui_settings_subtitle_controls_visibility_20260704/interaction_screenshot.png`.
- Evidence summary: `qml_loaded=true`, `qml_state_updated=true`,
  `fake_media_used=false`, `fake_content_used=false`,
  `typed_p7_harness=true`, `exit=normal`, `residual_process=false`.
- Snapshot evidence covers both no-subtitle initial state
  (`subtitleOptions=[]`, `subtitlePrimary=""`, `subtitleSecondary=""`) and the
  later harness state with subtitle options available.
- Touched QML non-ASCII scan returned no matches.

Current truthful limits:

- This hides irrelevant subtitle tuning controls. It does not add full P8
  styled subtitle rendering or PGS bitmap rendering.
- P5 hardware presenter/frame pacing and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Shelter Debug Shell Heap Guard

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- This is not a repeat of the old real-demux MP4/Shelter mismatch fix. The
  observed issue was a Debug CRT heap corruption dialog from
  `kivo_video_playback_shell.exe` while opening real Shelter media.
- Natural family: P8 subtitle frame bridge plus shell evidence verification.
- Backend route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `SubtitleFrameP8Bridge::copy_primary_text` now caps text copy size by the
  caller output buffer capacity as well as the local temporary buffer size.
- This removes a real fixed-buffer write risk in the P8 bridge. The current
  caller uses a 256-byte buffer, but the helper accepts `out_size` and must
  honor it.
- No UI/QML entry route changed. No Chinese source strings were added.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell kivo_p7c_command_state_tests kivo_p8d_source_privacy_tests kivo_p8e_frame_lease_tests kivo_p8f_text_font_privacy_tests -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p7c_command_state_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests)" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
```

Result:

- Debug playback shell build: passed.
- P7/P8 focused CTest: `4/4` passed.
- Expanded media-chain/P8/UI CTest: `16/16` passed.
- Shelter Debug playback evidence:
  `artifacts/p10/shelter_debug_heap_guard_20260704/playback_evidence.json`.
- Shelter Debug continuous evidence:
  `artifacts/p10/shelter_debug_continuous_heap_guard_20260704/continuous_playback_evidence.json`.
- Shelter Debug normal `--media` launch with programmatic window close:
  `artifacts/p10/shelter_debug_normal_close_heap_guard_20260704/stderr.txt`.
- The exact Shelter UNC media path is recorded in the evidence JSON
  `media_path` fields.
- All three Shelter shell runs exited with code `0`; no Debug CRT heap
  corruption dialog remained and no residual process remained.
- Playback evidence summary: `open_result=accepted`,
  `first_frame_decoded=true`, `frame_pixels_valid=true`,
  `video_codec=hevc`, `width=1920`, `height=804`, `errors=""`.
- Continuous evidence summary: `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`, `audio_output_started=true`, `audio_error=""`,
  `p8_embedded_packet_ingest_connected=true`,
  `p8_full_subtitle_render_connected=false`,
  `p8_pgs_bitmap_render_connected=false`.
- PGS probe warnings still appear in stderr. They are noisy but did not block
  playback or produce heap corruption in these verified Debug runs.

Current truthful limits:

- This validates the Debug shell against the reported heap-corruption scenario
  and fixes one P8 buffer-boundary risk.
- It does not implement PGS bitmap rendering.
- TrueHD passthrough remains unimplemented; current correct behavior is still
  fail-closed with DTS fallback where available.
- P5 hardware presenter/frame pacing remains unfinished.

## 2026-07-04 Update: Continuous Evidence Audio Probe Semantics

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Evidence semantics only, inside the existing shell continuous evidence family.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- Continuous playback evidence now separates audio switch probe state from final
  playback state.
- Existing final-looking fields such as `audio_track_after_switch`,
  `audio_codec_after_switch`, and `audio_frames_written_after_switch` now record
  the final restored state after a failed unsupported probe.
- New explicit probe fields record the transient selection:
  `audio_track_probe_target`, `audio_track_probe_selected`,
  `audio_codec_probe`, `audio_probe_failed`, `audio_probe_error`,
  `audio_track_restore_attempted`, `audio_track_restore_command_ok`,
  `audio_track_restored`, `audio_track_final`, and `audio_codec_final`.
- A restored unsupported TrueHD probe no longer pollutes `errors` when final
  playback returns to the prior healthy DTS track.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell kivo_player_runtime_adapter_tests kivo_playback_shell_interaction_test kivo_playback_shell_video_surface_geometry_test -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
```

Result:

- Debug playback shell and focused test targets built successfully.
- Focused CTest: `9/9` passed.
- Expanded media-chain/P8/UI CTest: `16/16` passed.
- Shelter Debug continuous evidence:
  `artifacts/p10/shelter_debug_audio_probe_final_20260704/continuous_playback_evidence.json`.
- Real local summary:
  `artifacts/p10/shelter_debug_audio_probe_final_20260704/real_playback_local_evidence.json`.
- Evidence summary: `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`, `audio_track_before_switch=4`,
  `audio_track_probe_target=2`, `audio_track_probe_selected=2`,
  `audio_codec_probe=truehd`, `audio_probe_failed=true`,
  `audio_track_restore_attempted=true`, `audio_track_restored=true`,
  `audio_track_final=4`, `audio_codec_final=dts`, `audio_error=""`,
  `errors=""`.
- No residual `kivo*`, `WerFault`, `vsjitdebugger`, `ctest`, `cmake`, or
  `MSBuild` process remained after the verification run.

Current truthful limits:

- This is evidence hygiene and restoration reporting only. It does not implement
  TrueHD passthrough.
- PGS bitmap rendering remains unimplemented.
- P5 hardware presenter/frame pacing remains unfinished.

## 2026-07-04 Update: Snapshot Binding Hides Stale Media Options

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI input binding only. No playback data-plane route change.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackSnapshotBinding.qml` now treats each Adapter snapshot as the
  authority for media option availability.
- When the latest snapshot has no `episodeOptions`, QML clears
  `episodeOptions` and `currentEpisode` instead of retaining stale values from a
  previous media item.
- When the latest snapshot has no `versionOptions`, QML clears `versionOptions`
  and `versionTechnicalRows` instead of retaining stale version metadata.
- This keeps the hidden More hub honest: Episodes and Versions only appear when
  the current session actually reports those options.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_snapshot_binding_stale_options_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_playback_snapshot_binding_stale_options_test|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test)" --output-on-failure
```

Result:

- Debug playback shell and new snapshot binding test target built successfully.
- Focused UI CTest: `4/4` passed.
- Expanded media-chain/P8/UI CTest: `17/17` passed.
- New coverage:
  `kivo_playback_snapshot_binding_stale_options_test` applies a snapshot with
  episode and version options, then a snapshot with empty media options, and
  verifies that the page state clears `episodeOptions`, `currentEpisode`,
  `versionOptions`, and `versionTechnicalRows`.

Current truthful limits:

- This is UI state hygiene. It does not add playlist discovery beyond the
  current Adapter snapshot data.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Settings Hide Irrelevant Playback Controls

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI settings visibility only. Commands remain wired through the existing QML
  to Adapter/P7 route.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `SettingsSheet.qml` now hides `Audio sync` when the current media/session does
  not report audio controls.
- `SettingsSheet.qml` now hides `Continue playing` when the current
  media/session does not report episode or playlist-next capability.
- `PlaybackOverlay.qml` derives `audioControlsAvailable` from current audio
  track state and derives `autoPlayNextControlsAvailable` from current episode
  and playlist-next state, then passes both into Settings.
- Speed remains available; subtitle tuning remains governed by the existing
  subtitle availability rule.
- The underlying `setAudioDelay` command path remains connected. This is
  presentation discipline only. The underlying `setAutoPlayNext` command path
  also remains connected.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_settings_sheet_visibility_test kivo_playback_snapshot_binding_stale_options_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_playback_settings_sheet_visibility_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_player_runtime_adapter_tests|kivo_p7h_local_media_first_frame_tests|kivo_p7c_command_state_tests|kivo_video_provider_local_file_read_test|kivo_video_provider_local_file_seek_test|kivo_video_provider_local_file_close_test|kivo_video_provider_local_file_basic_thread_safety_test|kivo_video_source_runtime_local_media_queue_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test)" --output-on-failure
```

Result:

- Debug playback shell and focused UI test targets built successfully.
- Focused UI CTest: `5/5` passed.
- Expanded media-chain/P8/UI CTest: `18/18` passed.
- New coverage:
  `kivo_playback_settings_sheet_visibility_test` verifies that `Audio sync` and
  subtitle sync rows stay hidden with no matching current media capability, and
  that `Continue playing` stays hidden until auto-play-next availability is set.
  It also verifies that Speed remains visible.

Current truthful limits:

- This is UI visibility polish only. It does not add new audio processing,
  playlist discovery, or subtitle rendering capability.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Sheet Shortcuts Respect Hidden Capability Gates

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI playback page shortcut and context-menu gating only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.qml` now treats subtitle, audio, episode, and version sheets as
  unavailable when the current Adapter snapshot has no corresponding options.
- Keyboard shortcuts now ignore unavailable sheets instead of opening empty UI.
- Right-click context-menu handlers now share the same page-level availability
  gate, so programmatic signal paths cannot bypass hidden menu items.
- Commands remain wired for available sheets. This is presentation discipline
  and does not remove deep capability paths.
- Removed the stray root-level `gafafafafafaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt`
  handoff dump because it violated the root file and language hygiene rules.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_sheet_shortcut_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/page/PlaybackPage.qml ui/CMakeLists.txt tests/ui_shell/playback_sheet_shortcut_visibility_test.cpp
```

Result:

- Debug playback shell and new sheet shortcut test target built successfully.
- Focused UI CTest: `6/6` passed.
- Expanded media-chain/P8/UI CTest: `18/18` passed.
- New coverage:
  `kivo_playback_sheet_shortcut_visibility_test` verifies that unavailable
  subtitle, audio, episode, and version shortcuts do not open sheets, and that
  each shortcut opens the expected sheet once matching options exist.

Current truthful limits:

- This is UI chrome gating only. It does not add playlist discovery, new codec
  support, hardware presentation, or full subtitle rendering.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Timeline Seekability Requires Real Duration

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI playback timeline input discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackTimeline.qml` now exposes a `seekable` state derived from
  `duration > 0`.
- When duration is unknown or invalid, the timeline is visually quiet, shows an
  unknown total time, and disables its mouse seek area instead of emitting a
  fake zero-second seek.
- When a real positive duration arrives, the existing seek/hover/preview path is
  enabled again.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_timeline_seekability_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackTimeline.qml ui/CMakeLists.txt tests/ui_shell/playback_timeline_seekability_test.cpp
```

Result:

- Debug playback shell and new timeline seekability test target built
  successfully.
- Focused UI CTest: `7/7` passed.
- Expanded media-chain/P8/UI CTest: `19/19` passed.
- New coverage:
  `kivo_playback_timeline_seekability_test` verifies that the timeline and its
  mouse seek area stay disabled with zero or negative duration, and re-enable
  when a positive duration is present.

Current truthful limits:

- This is UI interaction polish only. It does not add timeline ownership,
  seeking capability, video presentation, codec support, or subtitle rendering.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Hidden Shortcuts Stay Quiet Until Available

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI keyboard input discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackInputController.qml` now receives subtitle, audio, episode, and
  version availability from `PlaybackPage.qml`.
- Hidden sheet shortcuts now return unhandled while unavailable, so pressing an
  unavailable shortcut does not wake the overlay chrome and does not emit a
  sheet request.
- Once the matching option exists, the same shortcut still wakes chrome and
  emits the existing request path. Page-level sheet gates remain in place as a
  second guard.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_input_hidden_shortcut_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test)$" --output-on-failure
git diff --check -- ui/playback/input/PlaybackInputController.qml ui/playback/page/PlaybackPage.qml ui/CMakeLists.txt tests/ui_shell/playback_input_hidden_shortcut_test.cpp
```

Result:

- Debug playback shell and new hidden shortcut input test target built
  successfully.
- Focused UI CTest: `8/8` passed.
- Expanded media-chain/P8/UI CTest: `20/20` passed.
- New coverage:
  `kivo_playback_input_hidden_shortcut_test` verifies that an unavailable
  subtitle shortcut does not reveal overlay chrome and does not emit a request,
  then verifies the same shortcut works after subtitle availability is enabled.

Current truthful limits:

- This is UI keyboard/chrome polish only. It does not add new sheet content,
  playlist discovery, subtitle rendering, codec support, or backend capability.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Overlay Sheet Requests Share Capability Gates

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI overlay sheet gating only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now owns `sheetAvailable`,
  `openSheetIfAvailable`, and `toggleSheetIfAvailable` helpers.
- Bottom control sheet requests and More-sheet action requests now use the same
  overlay-level availability gate as the visible UI model.
- Programmatic overlay signals can no longer open subtitle, audio, episode, or
  version sheets when the current Adapter snapshot has no matching options.
- `settings` and `more` remain available. Available media-option sheets still
  open through the existing overlay state path.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_overlay_sheet_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml ui/CMakeLists.txt tests/ui_shell/playback_overlay_sheet_gate_test.cpp
```

Result:

- Debug playback shell and new overlay sheet gate test target built
  successfully.
- Focused UI CTest: `9/9` passed.
- Expanded media-chain/P8/UI CTest: `21/21` passed.
- New coverage:
  `kivo_playback_overlay_sheet_gate_test` verifies that unavailable media sheets
  cannot be opened from overlay helper paths, that settings remains available,
  and that available media sheets can still open/toggle normally.

Current truthful limits:

- This is UI overlay/chrome polish only. It does not add new media option
  discovery, sheet content, subtitle rendering, codec support, or backend
  capability.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Status Overlay Suppresses Playback Chrome

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI overlay status/chrome visibility only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now derives `chromeVisible` from
  `overlayState.overlayVisible && !overlayState.statusActive`.
- Top info, bottom controls, and subtitle control-lift visibility now consume
  `chromeVisible` instead of raw `overlayVisible`.
- Buffering, recovering, recovered, and error states keep the dark gradient and
  centered status UI but suppress regular playback chrome.
- Normal visible/paused/sheet states still show chrome through the existing
  overlay state path.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml ui/CMakeLists.txt tests/ui_shell/playback_status_chrome_visibility_test.cpp
```

Result:

- Debug playback shell and new status chrome visibility test target built
  successfully.
- Focused UI CTest: `10/10` passed.
- Expanded media-chain/P8/UI CTest: `22/22` passed.
- New coverage:
  `kivo_playback_status_chrome_visibility_test` verifies that normal visible
  overlay state shows top/bottom chrome, buffering status suppresses chrome, and
  clearing status restores chrome.

Current truthful limits:

- This is UI status/chrome polish only. It does not add buffering behavior,
  recovery behavior, presentation timing, codec support, subtitle rendering, or
  backend capability.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Seek Controls Follow Real Duration Availability

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI seek/skip presentation discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackInputController.qml` now gates left/right skip shortcuts behind
  `seekAvailable`.
- `PlaybackOverlay.qml` derives `seekAvailable` from real positive duration and
  passes it through bottom controls.
- `TransportControls.qml` hides and disables the 10-second back/forward buttons
  when duration is unknown or zero.
- `PlaybackContextMenu.qml` hides and disables the 10-second back/forward menu
  items under the same rule.
- `PlaybackPage.qml` passes the duration-derived gate to keyboard and context
  menu entry points, and still guards context-menu skip dispatch before sending
  adapter commands.
- `PlaybackIconSet.qml` redraws the 10-second skip glyphs with clearer small-size
  text placement so the restrained transport chrome reads as polished controls
  instead of test-only symbols.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_seek_availability_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test)$" --output-on-failure
git diff --check -- ui/playback/input/PlaybackInputController.qml ui/playback/overlay/TransportControls.qml ui/playback/overlay/BottomControlOverlay.qml ui/playback/overlay/PlaybackOverlay.qml ui/playback/menu/PlaybackContextMenu.qml ui/playback/page/PlaybackPage.qml ui/CMakeLists.txt tests/ui_shell/playback_seek_availability_test.cpp
```

Result:

- Debug playback shell and new seek availability test target built
  successfully.
- Focused UI CTest: `11/11` passed.
- Expanded media-chain/P8/UI CTest: `23/23` passed.
- After the skip glyph polish, the same focused UI and expanded media-chain/P8/UI
  CTest sets were rerun and remained `11/11` and `23/23` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI presentation gating only. It does not add new seek backend
  capability, timeline semantics, media probing, subtitle rendering, codec
  support, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Context Menu Respects Modal Playback States

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI page-level context menu discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.qml` now routes right-click opening through
  `requestContextMenu(x, y)`.
- The context menu opens only when no playback sheet is open and no status UI is
  active.
- Right-clicking while a sheet is open closes the sheet and does not open the
  context menu.
- Buffering, recovering, recovered, and error status states suppress the context
  menu, so regular playback commands do not appear on top of modal status UI.
- The normal available context menu path remains intact after status clears.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test)$" --output-on-failure
git diff --check -- ui/playback/page/PlaybackPage.qml ui/CMakeLists.txt tests/ui_shell/playback_context_menu_state_gate_test.cpp
```

Result:

- Debug playback shell and new context menu state gate test target built
  successfully.
- Focused UI CTest: `12/12` passed.
- Expanded media-chain/P8/UI CTest: `24/24` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI input/chrome polish only. It does not add context menu features,
  backend capability, media probing, subtitle rendering, codec support, or
  TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Keyboard Shortcuts Respect Modal Playback States

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI keyboard input discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackInputController.qml` now suppresses ordinary keyboard playback
  shortcuts while a playback sheet is open.
- Sheet-open state allows only `Escape`, which closes the sheet.
- Active status UI suppresses ordinary playback, seek, volume, and media-option
  shortcuts so commands do not run invisibly behind buffering/recovery/error
  presentation.
- Active status UI still allows `Escape` for back/exit behavior and `Open Video`
  for recovery/open-new-media behavior.
- Normal playback shortcuts resume after status clears.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_input_modal_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test)$" --output-on-failure
git diff --check -- ui/playback/input/PlaybackInputController.qml ui/CMakeLists.txt tests/ui_shell/playback_input_modal_state_gate_test.cpp
```

Result:

- Debug playback shell and new input modal state gate test target built
  successfully.
- Focused UI CTest: `13/13` passed.
- Expanded media-chain/P8/UI CTest: `25/25` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI keyboard/input polish only. It does not add backend capability,
  media probing, subtitle rendering, codec support, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Fullscreen Gesture Respects Modal Playback States

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI pointer gesture discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.qml` now routes double-click fullscreen changes through
  `requestFullscreenGesture()`.
- The gesture is suppressed while a playback sheet is open, keeping sheet state
  modal and quiet.
- The gesture is suppressed while status UI is active, so buffering, recovering,
  recovered, and error presentation cannot accidentally toggle fullscreen.
- The normal double-click path still dispatches `enterFullscreen`; after
  `fullscreenLike` is true, the same path dispatches `exitFullscreen`.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_fullscreen_gesture_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_fullscreen_gesture_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test)$" --output-on-failure
git diff --check -- ui/playback/page/PlaybackPage.qml ui/CMakeLists.txt docs/handoff/p10-real-playback-current-state.md tests/ui_shell/playback_fullscreen_gesture_state_gate_test.cpp
```

Result:

- Debug playback shell and new fullscreen gesture state gate test target built
  successfully.
- New test executable returned `exit=0`.
- Focused UI CTest: `14/14` passed.
- Expanded media-chain/P8/UI CTest: `26/26` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI gesture/input polish only. It does not add backend capability,
  media probing, subtitle rendering, codec support, presentation timing, or
  TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: More Hub Visibility Follows Real Available Controls

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI chrome exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now computes `moreHubAvailable` from real available
  subtitle, audio, episode, version, custom action, and advanced setting state.
- The bottom `More` button is no longer shown when the hub would only expose
  empty/default UI.
- `MorePlaybackSheet.qml` now respects the incoming `actions` list and includes
  only meaningful custom actions.
- `Settings` remains wired, but it is exposed through the More hub only when
  advanced setting state is relevant, such as available subtitle/audio controls,
  playlist continuation controls, or a non-default playback speed.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_more_hub_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml ui/playback/overlay/PlaybackToolControls.qml ui/playback/sheets/MorePlaybackSheet.qml ui/CMakeLists.txt tests/ui_shell/playback_more_hub_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and new More hub visibility test target built
  successfully.
- New test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI chrome exposure polish only. It does not add backend capability,
  media probing, subtitle rendering, codec support, presentation timing, or
  TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Audio Settings Availability Accepts Real Track Labels

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI availability-state correction only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now treats a selected audio track as present when the
  runtime state carries either a positive numeric track id or a non-empty
  non-zero display label.
- This fixes the previous false negative where labels such as `DTS 5.1` became
  `NaN` under `Number(...)` and could incorrectly hide the audio sync row and
  the More/Settings entry.
- Empty audio labels and zero-like values still keep advanced audio settings
  hidden.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_more_hub_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml tests/ui_shell/playback_more_hub_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated More hub visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI availability-state polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: More Hub Custom Actions Stay Hidden Until Meaningful

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI More hub route and exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now counts only meaningful custom More actions when
  deciding whether the More hub should appear.
- `MorePlaybackSheet.qml` uses the same non-empty label/action-name rule before
  adding incoming custom actions to the visible hub list.
- Blank custom action placeholders no longer expose an empty More button.
- Meaningful custom actions still dispatch through `sheetActionRequested`, so
  deeper routes remain wired without forcing the default chrome to expose them.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_more_hub_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml ui/playback/sheets/MorePlaybackSheet.qml tests/ui_shell/playback_more_hub_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated More hub visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI route/exposure polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Context Menu Hides Playback Commands Without Media

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI context-menu exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.qml` now derives a `mediaContextAvailable` signal from positive
  duration, visible title, non-empty real playback state, or available media
  option inventory.
- `PlaybackContextMenu.qml` keeps Open Video/Open Folder available in empty
  state, but hides `Play / Pause` and the playback command separator until a
  real media context exists.
- Seek commands remain independently governed by real seek availability.
- This keeps the recovery/open route accessible while preventing empty playback
  chrome from exposing commands that cannot act on a current item.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_context_menu_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/menu/PlaybackContextMenu.qml ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_context_menu_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated context menu state gate test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI context-menu exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: FFmpeg PGS Probe Noise Filter

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Product stderr hygiene only at the FFmpeg adapter boundary.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- Added `ffmpeg_product_log_filter` under
  `backend/src/third_party_adapter/ffmpeg_adapter`.
- `FfmpegAdapter::open_file(...)` and `FfmpegAdapter::open_custom_io(...)`
  now install that process-local FFmpeg log callback before opening/probing
  media.
- The filter only suppresses known non-actionable Matroska probe messages:
  stream-rate estimation warnings, PGS `unspecified size` warnings, and the
  paired `analyzeduration` / `probesize` suggestion.
- Other FFmpeg errors and warnings still flow through the default FFmpeg log
  callback.
- Evidence remains truthful: P8 embedded packet ingest can be connected, but
  PGS bitmap rendering is still not claimed.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell kivo_real_demux_decode_integration_test kivo_player_runtime_adapter_tests -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_mapper_track_label_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/ffmpeg_probe_log_filter_shelter_20260705 --media "<Shelter UNC directory>" --duration-ms 6000 --continuous-close-ms 500
git diff --check -- backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_product_log_filter.hpp backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_product_log_filter.cpp backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.cpp backend/src/third_party_adapter/ffmpeg_adapter/ffmpeg_custom_io.cpp backend/CMakeLists.txt
```

Result:

- Debug playback shell, real demux/decode integration test, and adapter tests
  built successfully.
- Expanded media-chain/P8/UI CTest: `28/28` passed.
- Shelter short evidence:
  `artifacts/p10/ffmpeg_probe_log_filter_shelter_20260705/continuous_playback_evidence.json`.
- Shelter stderr:
  `artifacts/p10/ffmpeg_probe_log_filter_shelter_20260705/stderr.txt`
  was 0 bytes. No `not enough frames`, `Could not find codec parameters`,
  `analyzeduration`, or `probesize` lines remained.
- Evidence stayed truthful: `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`, `audio_output_started=true`, `audio_error=""`,
  `p8_pgs_bitmap_render_connected=false`,
  `subtitle_status=partial_embedded_packet_ingest_no_bitmap_render`.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is product stderr/probe-noise hygiene only. It does not add subtitle
  rendering, codec support, audio sync implementation, hardware presentation,
  media probing capability, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Product UI Hides Unsupported PGS Subtitles

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Product subtitle inventory exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- Added `local_media_subtitle_display_filter` under the P7 local media family.
- Local media track inventory now copies only currently displayable embedded
  subtitle codecs into the Adapter/QML product subtitle options:
  `subrip`, `srt`, `ass`, `ssa`, `webvtt`, `mov_text`, and `text`.
- Bitmap subtitle codecs such as PGS, VobSub/DVD subtitle, and DVB subtitle stay
  discoverable in backend probe/evidence, but are not exposed as selectable
  product subtitle options until their real render path exists.
- This keeps the Infuse-style product surface quiet: a user does not see a
  Subtitles entry for Shelter PGS tracks that cannot currently render.
- Continuous playback evidence now records
  `subtitle_product_option_count` separately from `subtitle_stream_count`.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell kivo_p7_local_media_subtitle_display_filter_tests kivo_player_runtime_adapter_tests -- /m:1 /v:minimal
build\vs-debug-ffmpeg\backend\Debug\kivo_p7_local_media_subtitle_display_filter_tests.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_p7_local_media_subtitle_display_filter_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_mapper_track_label_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/ui_hide_unsupported_pgs_subtitles_final_shelter_20260705 --media "<Shelter UNC directory>" --duration-ms 6000 --continuous-close-ms 500
```

Result:

- Build: passed.
- New deterministic subtitle display filter test: `filter_exit=0`.
- Expanded media-chain/P8/UI CTest: `29/29` passed.
- Shelter short evidence:
  `artifacts/p10/ui_hide_unsupported_pgs_subtitles_final_shelter_20260705/continuous_playback_evidence.json`.
- Shelter playback: `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`, `audio_output_started=true`, `audio_error=""`,
  `audio_track_restored=true`.
- Subtitle exposure proof:
  `subtitle_stream_count=5`, `subtitle_product_option_count=0`,
  `p8_subtitle_track_inventory_discovered=true`,
  `p8_subtitle_packets_submitted=0`, `p8_pgs_bitmap_render_connected=false`,
  `subtitle_status=deferred`.
- Shelter stderr:
  `artifacts/p10/ui_hide_unsupported_pgs_subtitles_final_shelter_20260705/stderr.txt`
  was 0 bytes. No Matroska/PGS probe noise remained.

Current truthful limits:

- This is product inventory exposure discipline only. It does not implement PGS
  bitmap rendering, full subtitle rendering, TrueHD passthrough, audio sync
  closure, or hardware presenter/frame pacing.
- P8 still discovers Shelter subtitle streams in backend evidence, but the
  product UI hides unsupported bitmap subtitle options until their real render
  path is implemented.

## 2026-07-05 Update: Closed Snapshot State Hides Stale Seek Controls

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI page-level stale-snapshot gating only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.qml` now treats explicit `NotCreated` and `Closed` playback
  states as closed media context, even if an older snapshot still has duration,
  title, or option data.
- Page-level input seek availability and context-menu seek availability now
  require both trusted media context and positive duration.
- Context-menu skip handlers also fail closed when media context is closed.
- `playback_context_menu_state_gate_test.cpp` now covers the stale duration plus
  `Closed` snapshot case and verifies that Open Video/Open Folder return while
  Play/Pause, skip controls, More, and input seek stay hidden.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_context_menu_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_context_menu_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated context menu state gate test target built
  successfully.
- Updated test executable returned `context_exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI stale-snapshot gating only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Empty Page Fullscreen Gesture Fails Closed

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI page-level fullscreen exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.requestFullscreenThroughAdapter()` now fails closed when there is
  no trusted media context and the page is not already fullscreen-like.
- `PlaybackPage.requestFullscreenGesture()` no longer accepts an empty-page
  double-click as a fullscreen request.
- Fullscreen exit remains allowed when `fullscreenLike` is true, preserving the
  escape route from an existing fullscreen state.
- `playback_fullscreen_gesture_state_gate_test.cpp` now verifies empty-page
  fullscreen gesture suppression before allowing the same gesture once duration
  proves media context.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_fullscreen_gesture_state_gate_test kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_fullscreen_gesture_state_gate_test.exe
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_context_menu_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_context_menu_state_gate_test.cpp tests/ui_shell/playback_fullscreen_gesture_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated UI state gate test targets built
  successfully.
- Updated test executables returned `fullscreen_exit=0` and `context_exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI fullscreen exposure polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: More Hub Hides Unlabeled Custom Actions

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI More hub exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` and `MorePlaybackSheet.qml` now require custom More
  actions to provide both visible `label` and stable `actionName` before they
  count as meaningful UI actions.
- This prevents blank but clickable More rows from appearing when a deep action
  is wired without user-facing copy.
- Built-in More routes stay hidden until their existing capability gates pass.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_more_hub_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml ui/playback/sheets/MorePlaybackSheet.qml tests/ui_shell/playback_more_hub_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated More hub visibility test target built
  successfully.
- Updated test executable returned `more_hub_exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI More hub exposure polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Error Status Shows One Primary Recovery Action

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI error-state action exposure only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `StatusOverlay.qml` now hides `Open Video` when retry is available.
- Playback-context errors show one primary recovery action: `Retry`.
- No-media/open failures still show `Open Video`, preserving the manual open
  route without presenting two competing error actions during playback.
- `playback_status_chrome_visibility_test.cpp` now verifies that retry-enabled
  playback errors do not expose the Open Video button.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/StatusOverlay.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated test executable returned `status_chrome_exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI error-state exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Track Labels Hide Raw Stream IDs

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI snapshot label polish only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `playback_shell_snapshot_mapper.cpp` now labels audio, subtitle, and video
  options by display order (`Audio 1`, `Audio 2`, `Subtitle 1`, etc.) instead of
  exposing raw container stream ids in the visible label.
- The real `trackId` and `id` fields still carry the original stream ids, so
  selection commands continue to target the correct backend track.
- This does not invent language, codec, or channel metadata. It only prevents
  raw stream ids such as `Audio 2` for the first audio option from leaking into
  the user-facing sheet.
- Added `kivo_playback_snapshot_mapper_track_label_test` to verify both the
  user-facing ordinal labels and preserved backend ids.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_snapshot_mapper_track_label_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_snapshot_mapper_track_label_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(snapshot_mapper_track_label|more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_mapper_track_label_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/CMakeLists.txt ui/shell/playback_shell_snapshot_mapper.cpp tests/ui_shell/playback_snapshot_mapper_track_label_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- CMake regenerated because `ui/CMakeLists.txt` added the mapper label test.
- Debug playback shell and new mapper label test target built successfully.
- New mapper label test returned `snapshot_mapper_exit=0`.
- Focused UI CTest: `16/16` passed.
- Expanded media-chain/P8/UI CTest: `28/28` passed.

Current truthful limits:

- This is UI track-label polish only. It does not add language, codec, channel,
  subtitle rendering, media probing, audio sync implementation, presentation
  timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Page Media Context Ignores Stale Options

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI page-level media-context truthfulness only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackPage.qml` no longer treats subtitle, audio, episode, or version
  option lists as proof that media context exists.
- Page-level `mediaContextAvailable` is now based on stronger media identity
  signals: positive duration, visible title metadata, or a non-closed playback
  state.
- Stale option lists can still flow to deep sheet routes once real media context
  exists, but they no longer light playback chrome, More, or the context-menu
  playback surface by themselves.
- `playback_context_menu_state_gate_test.cpp` now verifies that stale subtitle
  options alone keep the page in the no-media context-menu mode.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_context_menu_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_context_menu_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated context-menu state gate test target built
  successfully.
- Updated context-menu state gate test returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene: only existing LF/CRLF conversion warnings were reported for touched
  QML and handoff files, the added diff scan found no non-ASCII text, and no
  residual Kivo/CTest/CMake/MSBuild or debugger processes remained.

Current truthful limits:

- This is UI page-level context truthfulness polish only. It does not add
  backend capability, media probing, audio sync implementation, subtitle
  rendering, codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Timeline Defaults Fail Closed

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI playback timeline exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackTimeline.qml` now has an explicit `timelineAvailable` gate that
  defaults to `false`.
- A bare timeline no longer exposes `0:00`, `--:--`, a dim progress bar, or an
  enabled seek mouse area just because a component was instantiated.
- `BottomControlOverlay.qml` opens the timeline gate only from the existing
  `seekAvailable` path, so real playback seek support stays wired while the
  standalone default remains quiet.
- `playback_timeline_seekability_test.cpp` now verifies that duration alone is
  not enough to make the timeline visible or seekable; the explicit timeline
  availability gate must also be true.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_timeline_seekability_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_timeline_seekability_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackTimeline.qml ui/playback/overlay/BottomControlOverlay.qml tests/ui_shell/playback_timeline_seekability_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated timeline seekability test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene: only existing LF/CRLF conversion warnings were reported for touched
  QML files, and the added diff scan found no non-ASCII text.

Current truthful limits:

- This is UI timeline exposure polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Fullscreen Chrome Defaults Fail Closed

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI playback chrome exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackToolControls.qml` now has an explicit `fullscreenAvailable` gate
  that defaults to `false`.
- The fullscreen tool button no longer appears as a lone control in the
  no-media playback chrome.
- `BottomControlOverlay.qml` opens the fullscreen gate from the existing
  playback-available media-control path.
- If the shell is already fullscreen, the button remains visible so the user can
  exit fullscreen even if playback availability drops.
- `playback_status_chrome_visibility_test.cpp` now verifies the hidden default,
  playback-enabled visibility, and fullscreen exit fallback.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackToolControls.qml ui/playback/overlay/BottomControlOverlay.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene: only existing LF/CRLF conversion warnings were reported for touched
  QML and handoff files, the added diff scan found no non-ASCII text, and no
  residual Kivo/CTest/CMake/MSBuild or debugger processes remained.

Current truthful limits:

- This is UI fullscreen chrome exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Bottom Chrome Requires Playback Context

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI bottom chrome exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now has explicit `bottomChromeAvailable` and
  `bottomChromeVisible` gates.
- The bottom gradient and bottom control shell no longer light up from the
  global overlay state when there is no playback context.
- `seekAvailable` now requires both playback availability and a positive
  duration, preventing stale duration values from exposing the timeline.
- More/Settings availability now requires playback availability, preventing
  stale subtitle text, track labels, options, or custom actions from exposing
  bottom chrome without a valid playback context.
- Fullscreen exit remains a bottom-chrome escape hatch when already fullscreen.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_playback_more_hub_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp tests/ui_shell/playback_more_hub_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status/more visibility test targets built
  successfully.
- Updated status chrome visibility test returned `exit=0`.
- Updated More hub visibility test returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene: only existing LF/CRLF conversion warnings were reported for touched
  QML and handoff files, the added diff scan found no non-ASCII text, and no
  residual Kivo/CTest/CMake/MSBuild or debugger processes remained.

Current truthful limits:

- This is UI bottom chrome exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Context Menu Hides Open Entries During Playback

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI context-menu exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackContextMenu.qml` now has an explicit `openActionsAvailable` gate
  that defaults to `true` for standalone/no-media usage.
- `PlaybackPage.qml` sets that gate to `false` when a media context exists, so
  Open Video and Open Folder no longer occupy the top of the right-click menu
  during playback.
- The context-menu separator now appears only when it separates visible groups,
  avoiding a leading separator after open entries are hidden.
- Open-file and open-folder signals remain wired; this is visibility discipline,
  not route removal.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_context_menu_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/menu/PlaybackContextMenu.qml ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_context_menu_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated context-menu state gate test target built
  successfully.
- Updated context-menu state gate test returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene: only existing LF/CRLF conversion warnings were reported for touched
  QML and handoff files, the added diff scan found no non-ASCII text, and no
  residual Kivo/CTest/CMake/MSBuild or debugger processes remained.

Current truthful limits:

- This is UI context-menu exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Volume Control Defaults To No Media

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI volume component-default exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `VolumeInlineControl.qml` now defaults `volumeAvailable` to `false`.
- The bare volume component no longer shows or enables its mute button without
  an explicit media context.
- `BottomControlOverlay.qml` passes `mediaControlsVisible` into
  `volumeAvailable`, so the normal playback page still shows volume controls
  when real media context exists.
- `playback_status_chrome_visibility_test.cpp` now covers the bare component
  default and the playback overlay integration gate.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated status chrome visibility test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI component-default polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Transport Controls Default To No Media

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI transport component-default exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `TransportControls.qml` now defaults `playbackAvailable` to `false`.
- A bare transport control instance no longer shows Play/Pause without an
  explicit playback context.
- `BottomControlOverlay.qml` already passes its gated `playbackAvailable`
  value, so the normal playback page still shows Play/Pause when media context
  exists.
- `playback_seek_availability_test.cpp` now covers the bare transport default
  and explicit opt-in behavior.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_seek_availability_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_seek_availability_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
```

Result:

- Debug playback shell and updated seek availability test target built
  successfully.
- Updated seek availability test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI component-default polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Settings Speed Row Requires Playback Context

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI Settings sheet component-default exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `SettingsSheet.qml` now defaults the Speed row to hidden through
  `playbackSpeedControlsAvailable=false`.
- `PlaybackOverlay.qml` explicitly enables that row from `playbackAvailable`,
  so the normal playback Settings route still shows speed controls when the
  shell has real media context.
- A bare Settings sheet or a parent that forgets to pass playback context no
  longer exposes an action that cannot operate on media.
- `playback_settings_sheet_visibility_test.cpp` now covers hidden-by-default,
  explicit opt-in, and return-to-hidden behavior for the Speed row.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_settings_sheet_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_settings_sheet_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
```

Result:

- Debug playback shell and updated Settings sheet visibility test target built
  successfully.
- Updated Settings sheet visibility test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI component-default polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: More Sheet Defaults Fail Closed

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI More sheet component-default exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `MorePlaybackSheet.qml` now defaults `settingsAvailable` to `false`.
- A bare More sheet no longer exposes Settings unless a parent explicitly
  enables it through the existing gate.
- `PlaybackOverlay.qml` still passes `advancedSettingsAvailable`, so the normal
  playback More/Settings route remains wired when real playback context or
  meaningful settings context exists.
- `playback_more_hub_visibility_test.cpp` now covers the bare component default
  and the explicit Settings opt-in.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_more_hub_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
```

Result:

- Debug playback shell and updated More hub visibility test target built
  successfully.
- Updated More hub visibility test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI component-default polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Top Chrome Requires Visible Metadata

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI top-chrome exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now derives `topInfoAvailable` from visible title or
  season/episode metadata.
- The top info row and top dark gradient use `topChromeVisible`, so empty-title
  playback surfaces do not show a lone top Back affordance or dim the video at
  the top edge.
- When title or season/episode metadata exists, the top info row and top
  gradient still appear with the normal chrome.
- Bottom playback chrome, More hub gates, context-menu gates, subtitle/status
  suppression, and adapter routing are unchanged.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated status chrome visibility test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.

Current truthful limits:

- This is UI presentation polish only. It does not add backend capability, media
  probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: More Settings Requires Playback Context

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI More/Settings exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` no longer lets a non-default `playbackSpeed` expose the
  More hub or Settings sheet while the shell has no playback context.
- Once `playbackAvailable` is true, a non-default speed still exposes Settings
  through the existing More/settings gate.
- Deep Settings routes remain wired, but empty playback surfaces stay quiet and
  do not advertise controls that cannot operate on media yet.

Verification:

```text
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_more_hub_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml tests/ui_shell/playback_more_hub_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Updated More hub visibility test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI exposure polish only. It does not add backend capability, media
  probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Keyboard Playback Commands Require Media Context

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI keyboard-command exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackInputController.qml` now has a `playbackAvailable` gate for ordinary
  media commands.
- Empty playback state now suppresses keyboard Play/Pause, volume step, and
  mute commands, preventing adapter-missing playback noise before a real media
  context exists.
- Open Video and fullscreen remain available as entry/window commands.
- Seek and media-option sheet shortcuts continue to use their existing specific
  availability gates.
- `PlaybackPage.qml` binds the input gate to the existing
  `mediaContextAvailable` state used by the context menu.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_input_modal_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_input_modal_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/input/PlaybackInputController.qml ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_input_modal_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated input modal state gate test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI keyboard-command exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Bottom Chrome Hides Empty Playback Controls

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI bottom-chrome exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackOverlay.qml` now accepts a `playbackAvailable` signal from
  `PlaybackPage.qml` and passes it to the bottom chrome.
- `BottomControlOverlay.qml` separates ordinary media controls from window/tool
  controls through `mediaControlsVisible`, and separates timeline visibility
  through `timelineVisible`.
- Empty playback state no longer exposes the bottom Play/Pause, volume, or
  non-seekable timeline chrome.
- Fullscreen and meaningful More/tool entry points remain available, so deeper
  routes can stay wired without showing empty media controls.
- The timeline remains hidden until `seekAvailable` is true, avoiding a visible
  `--:--` scrubber before duration is known.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/PlaybackOverlay.qml ui/playback/page/PlaybackPage.qml ui/playback/overlay/BottomControlOverlay.qml ui/playback/overlay/TransportControls.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI bottom-chrome exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Empty Error State Hides Retry

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI error-action exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `StatusOverlay.qml` now has a `retryAvailable` gate.
- `PlaybackOverlay.qml` binds that gate to the same `playbackAvailable` signal
  used by the bottom chrome.
- Empty error state keeps `Open Video` available as the real recovery entry,
  but hides `Retry` when there is no media context to retry.
- Once a real playback context exists, `Retry` becomes visible again and still
  dispatches through the existing retry route.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/StatusOverlay.qml ui/playback/overlay/PlaybackOverlay.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI error-action exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-04 Update: Error Detail Avoids Duplicate Generic Copy

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI error-copy presentation only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `StatusOverlay.qml` now separates the generic error title from optional error
  detail text.
- Empty or whitespace-only error details stay hidden.
- The default generic detail `Playback unavailable` is no longer repeated under
  the identical heading.
- Specific error details still render under the heading and remain testable
  through a stable `statusErrorDetailText` object name.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/StatusOverlay.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI error-copy polish only. It does not add backend capability, media
  probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Status Overlay Suppresses Subtitle Text

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI subtitle/status presentation only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `SubtitleOverlay.qml` now exposes a `subtitleVisible` policy.
- Subtitle text remains visible during ordinary playback chrome/sheet states,
  but hides while a status overlay is active.
- This prevents stale subtitle text from competing with buffering/error/recovery
  states when the status layer has taken over the screen.
- `PlaybackOverlay.qml` gives the subtitle overlay a stable object name so the
  policy is covered by UI shell tests.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_status_chrome_visibility_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/overlay/SubtitleOverlay.qml ui/playback/overlay/PlaybackOverlay.qml tests/ui_shell/playback_status_chrome_visibility_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated status chrome visibility test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI subtitle/status polish only. It does not add backend capability,
  media probing, audio sync implementation, subtitle rendering, codec support,
  presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Shell Visible Copy Uses Translation Hooks

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI shell visible-copy/i18n readiness only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackShellRoot.qml` now wraps the visible shell title, file dialog title,
  folder dialog title, and dialog filter labels in `qsTr(...)`.
- The manual Open Video and Open Folder routes stay wired exactly as before.
- This removes hard-coded visible shell copy from the default playback shell
  surface without changing runtime behavior.

Verification:

```text
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/shell/PlaybackShellRoot.qml docs/handoff/p10-real-playback-current-state.md
```

Result:

- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI shell-copy/i18n-readiness polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Context Menu Collapses Media Options Into More

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- UI context-menu exposure discipline only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- `PlaybackContextMenu.qml` no longer exposes separate top-level Subtitles,
  Audio, Episodes, and Versions entries.
- The context menu now uses one `More` entry when the existing More hub has
  meaningful content.
- `PlaybackPage.qml` routes that context-menu `More` action through the same
  `PlaybackOverlay.openSheetIfAvailable("more")` gate used by normal chrome.
- Deep routes remain wired through the More hub, but the default right-click
  surface is quieter and closer to the hidden-control playback model.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_playback_context_menu_state_gate_test kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Debug\kivo_playback_context_menu_state_gate_test.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "kivo_playback_(more_hub_visibility|fullscreen_gesture_state_gate|input_modal_state_gate|context_menu_state_gate|seek_availability|status_chrome_visibility|overlay_sheet_gate|input_hidden_shortcut|timeline_seekability|sheet_shortcut_visibility|settings_sheet_visibility|snapshot_binding_stale_options|shell_qml_load|shell_interaction|shell_video_surface_geometry)_test" --output-on-failure
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
git diff --check -- ui/playback/menu/PlaybackContextMenu.qml ui/playback/page/PlaybackPage.qml tests/ui_shell/playback_context_menu_state_gate_test.cpp docs/handoff/p10-real-playback-current-state.md
```

Result:

- Debug playback shell and updated context menu state gate test target built
  successfully.
- Updated test executable returned `exit=0`.
- Focused UI CTest: `15/15` passed.
- Expanded media-chain/P8/UI CTest: `27/27` passed.
- Hygiene:
  `git diff --check` reported only existing LF/CRLF conversion warnings, added
  diff scan found no non-ASCII text, and no residual Kivo/CTest/CMake/MSBuild or
  debugger processes remained.

Current truthful limits:

- This is UI context-menu exposure polish only. It does not add backend
  capability, media probing, audio sync implementation, subtitle rendering,
  codec support, presentation timing, or TrueHD passthrough.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-05 Update: Product UI Hides Unsupported TrueHD Audio

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- P7 local-media product inventory only.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- Added a local-media audio display filter beside the existing subtitle display
  filter.
- P7 still records the full probed audio stream inventory from the backend, but
  the product-facing audio track options now exclude `truehd` and `mlp` until
  real passthrough or a formal compatible fallback is implemented.
- The preferred local-media audio track now comes from the displayable audio
  set. For the Shelter SMB sample this keeps DTS selected and prevents the UI
  from offering an unsupported TrueHD choice.
- Continuous and real-local evidence now record `audio_product_option_count` so
  reviewers can distinguish backend stream discovery from product options.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_p7_local_media_audio_display_filter_tests kivo_p7_local_media_subtitle_display_filter_tests kivo_p7h_local_media_first_frame_tests kivo_player_runtime_adapter_tests kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\backend\Debug\kivo_p7_local_media_audio_display_filter_tests.exe
build\vs-debug-ffmpeg\backend\Debug\kivo_p7_local_media_subtitle_display_filter_tests.exe
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "^(kivo_p7_local_media_audio_display_filter_tests|kivo_p7_local_media_subtitle_display_filter_tests|kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_provider_local_file_(read|seek|close|basic_thread_safety)_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_p8d_source_privacy_tests|kivo_p8e_frame_lease_tests|kivo_p8f_text_font_privacy_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test|kivo_playback_snapshot_mapper_track_label_test|kivo_playback_snapshot_binding_stale_options_test|kivo_playback_settings_sheet_visibility_test|kivo_playback_sheet_shortcut_visibility_test|kivo_playback_timeline_seekability_test|kivo_playback_input_hidden_shortcut_test|kivo_playback_overlay_sheet_gate_test|kivo_playback_status_chrome_visibility_test|kivo_playback_seek_availability_test|kivo_playback_context_menu_state_gate_test|kivo_playback_input_modal_state_gate_test|kivo_playback_fullscreen_gesture_state_gate_test|kivo_playback_more_hub_visibility_test)$" --output-on-failure
kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/ui_hide_unsupported_truehd_audio_final_shelter_20260705 --media "<UNC Shelter 2026 directory>" --duration-ms 6000 --continuous-close-ms 500
```

Result:

- Debug build completed successfully.
- Direct audio/subtitle display filter tests returned `exit=0`.
- Focused backend/UI CTest: `30/30` passed.
- Shelter SMB evidence:
  `artifacts/p10/ui_hide_unsupported_truehd_audio_final_shelter_20260705/continuous_playback_evidence.json`.
- Evidence summary:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`,
  `audio_error=""`,
  `audio_stream_count=3`,
  `audio_product_option_count=1`,
  `subtitle_stream_count=5`,
  `subtitle_product_option_count=0`,
  `p8_pgs_bitmap_render_connected=false`,
  and captured stderr was 0 bytes.

Current truthful limits:

- This is product inventory exposure discipline. It does not implement TrueHD
  passthrough or decode support.
- TrueHD/MLP remain fail-closed and hidden from the product track chooser.
- P5 hardware presenter/frame pacing, full P8 subtitle rendering, PGS bitmap
  rendering, and TrueHD passthrough remain unfinished.

## 2026-07-06 Update: Shelter Seek Audio Reset And UI-Stall Lock Split

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Real Shelter SMB playback issue at about 60s: visible stall for a few seconds,
  audio disturbance, and generally uneven video.
- Playback route remains unchanged:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- P6 seek flush now resets the WASAPI shared PCM writer and fences stale decoded
  or in-flight PCM buffers by flush generation.
- P6 evidence now records `audio_flush_count`, `audio_writer_reset_count`, and
  `audio_stale_packets_discarded`.
- P7 background continuous pump now uses a smaller packet budget
  (`p7_background_pump_packet_budget=8`) instead of the prior 64-packet chunk.
- P7 continuous playback no longer holds the P7 state mutex while P5 performs
  HEVC demux/decode/RGBA work. A separate runtime mutex serializes video
  runtime operations, while the P7 state mutex only protects lightweight
  snapshots, frame-buffer handoff, audio/subtitle callback state, and timeline
  bookkeeping.
- Shell evidence now records P7 pump timing and video surface timing:
  `p7_background_pump_*`, `surface_refresh_*`, and `surface_paint_*`.
- This specifically addresses UI stalls caused by frame refresh/query calls
  waiting behind a long P7 pump. It does not claim hardware frame pacing or GPU
  presentation completion.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_graph kivo_video_player_runtime_adapter kivo_video_playback_shell kivo_player_runtime_adapter_tests kivo_playback_shell_interaction_test -- /m:1 /v:minimal
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_interaction_test|kivo_playback_shell_qml_load_test)" --output-on-failure
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_graph kivo_video_player_runtime_adapter kivo_video_playback_shell -- /m:1 /v:minimal
build\vs-debug-ffmpeg\ui\Release\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/p7_small_pump_shelter_release_20260706 --duration-ms 10000 --continuous-close-ms 500 --media "<UNC Shelter 2026 directory>"
ctest --test-dir build/vs-debug-ffmpeg -C Debug -R "(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7_local_media_audio_display_filter_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)" --output-on-failure
```

Result:

- Focused lock-split CTest: `4/4` passed.
- Focused media-chain/shell CTest: `8/8` passed.
- Release Shelter SMB evidence:
  `artifacts/p10/p7_small_pump_shelter_release_20260706/continuous_playback_evidence.json`.
- Evidence summary:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`,
  `audio_error=""`,
  `audio_output_started=true`,
  `audio_flush_count=1`,
  `audio_writer_reset_count=1`,
  `audio_stale_packets_discarded=1`,
  `decoded_frame_count=230`,
  `rendered_frame_count=230`,
  `p7_background_pump_packet_budget=8`,
  `p7_background_pump_max_elapsed_ms=90`,
  `surface_refresh_max_elapsed_ms=1`,
  `surface_paint_max_elapsed_ms=4`,
  `subtitle_product_option_count=0`,
  `p8_pgs_bitmap_render_connected=false`,
  `p8_embedded_packet_ingest_connected=false`,
  and captured stderr was 0 bytes.

Current truthful limits:

- P7 can still spend tens of milliseconds in the background P5 software
  HEVC/RGBA pump; the important improvement is that UI frame refresh no longer
  blocks behind that work.
- The current P5 video path is still software decode to RGBA plus Qt-painted
  surface. Hardware presenter/frame pacing, GPU upload/presentation, advanced
  HDR, full P8 subtitle rendering, PGS bitmap rendering, and TrueHD passthrough
  remain unfinished.

## 2026-07-06 Update: P5 Continuous D3D11 Texture Upload Evidence

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Continue from the Shelter seek/audio reset and P7 lock-split work without
  redoing the DTS fallback, TrueHD fail-closed, or unsupported PGS hiding fixes.
- Improve P5 truthfulness and performance posture for the Shelter 60s stall
  report while preserving the route:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- Added a P5 local-media continuous D3D11 upload owner under
  `video/video_plane/runtime/local_media`.
- Continuous decoded frames now upload into a D3D11 texture when a D3D11 device
  is available. The uploaded texture is reused across frames and reset on seek
  or close.
- `d3d11_uploaded` now reflects continuous playback when continuous playback is
  active; it no longer borrows the first-frame probe claim for that path.
- P5/P7/query/shell evidence now record:
  `d3d11_upload_available`, `d3d11_upload_initialized`,
  `d3d11_frames_uploaded`, `d3d11_upload_failures`,
  `d3d11_upload_last_elapsed_ms`, `d3d11_upload_max_elapsed_ms`,
  `d3d11_device_type`, `d3d11_upload_error`, and
  `p5_continuous_d3d11_upload_connected`.
- The existing D3D11 device/upload adapters are now quiet during normal
  operation so continuous evidence stdout/stderr are not polluted by per-frame
  upload logs.
- Shell layer status explicitly says the presentation path is still
  `qt_painted_surface` and that hardware presenter/frame pacing is not yet
  connected.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_cinema_engine kivo_video_plane_local_media_runtime kivo_video_playback_graph kivo_video_player_runtime_adapter kivo_video_playback_shell kivo_d3d11_first_frame_test kivo_presenter_boundary_first_frame_test kivo_p7h_local_media_first_frame_tests kivo_player_runtime_adapter_tests kivo_playback_shell_qml_load_test kivo_playback_shell_interaction_test
ctest --test-dir build/vs-debug-ffmpeg -C Debug --output-on-failure -R "^(kivo_d3d11_first_frame_test|kivo_presenter_boundary_first_frame_test|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)$"
ctest --test-dir build/vs-debug-ffmpeg -C Debug --output-on-failure -R "^(kivo_real_demux_decode_integration_test|kivo_decoded_audio_frame_converter_test|kivo_video_source_runtime_local_media_queue_tests|kivo_p7_local_media_audio_display_filter_tests|kivo_p7h_local_media_first_frame_tests|kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)$"
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell
build\vs-debug-ffmpeg\ui\Release\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/p5_continuous_d3d11_upload_shelter_release_20260706 --duration-ms 10000 --continuous-close-ms 500 --media "<UNC Shelter 2026 directory>"
```

Result:

- Focused D3D/P7/UI CTest: `6/6` passed.
- Focused media-chain/shell CTest: `8/8` passed.
- Release Shelter SMB evidence:
  `artifacts/p10/p5_continuous_d3d11_upload_shelter_release_20260706/continuous_playback_evidence.json`.
- Evidence summary:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`,
  `audio_error=""`,
  `audio_output_started=true`,
  `audio_frames_decoded=918`,
  `audio_frames_written=469664`,
  `decoded_frame_count=218`,
  `rendered_frame_count=218`,
  `d3d11_device_type=hardware`,
  `d3d11_uploaded=true`,
  `d3d11_frames_uploaded=218`,
  `d3d11_upload_failures=0`,
  `d3d11_upload_max_elapsed_ms=12`,
  `p5_continuous_d3d11_upload_connected=true`,
  `p5_hardware_presenter_connected=false`,
  `p5_presentation_path=qt_painted_surface`,
  `p7_background_pump_packet_budget=8`,
  `p7_background_pump_max_elapsed_ms=82`,
  `surface_refresh_max_elapsed_ms=1`,
  `surface_paint_max_elapsed_ms=7`,
  `audio_product_option_count=1`,
  `subtitle_product_option_count=0`,
  `p8_pgs_bitmap_render_connected=false`,
  `p8_embedded_packet_ingest_connected=false`,
  and captured stdout/stderr were both 0 bytes.

Checkpoint truthful limits:

- This was continuous GPU texture upload evidence, not full GPU presentation.
- At this checkpoint UI display still used the Qt painted surface. The later
  shell scene graph texture update below supersedes that specific presentation
  note. Native swapchain presentation, hardware frame pacing, advanced HDR,
  full P8 subtitle rendering, PGS bitmap rendering, and TrueHD passthrough
  remained unfinished.

## 2026-07-06 Update: Shell Scene Graph Texture Video Surface

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- Continue from the P5 continuous D3D11 upload work. This change improves the
  shell presentation path without claiming a native swapchain presenter.
- Playback route remains:
  `QML PlaybackPage -> PlaybackShellRuntimeAdapterBridge -> PlayerRuntimeAdapter -> P7 PlaybackSession -> P2/P3/P4/P5/P6/P8`.

Correction:

- Replaced the shell video surface implementation from `QQuickPaintedItem` and
  `QPainter::drawImage` to a `QQuickItem` that updates a Qt scene graph texture
  node from the latest RGBA frame.
- The QML-facing item name and bridge attachment path stay unchanged, so QML
  still does not call P4/P5/P6/P8 internals directly.
- Evidence now reports `p5_presentation_path=qt_scenegraph_texture`.
- `p5_hardware_presenter_connected` remains `false`; the new presentation path
  is a Qt scene graph texture path, not a D3D swapchain presenter.
- Existing surface timing fields remain present. `surface_paint_*` now measures
  scene graph node/texture update time for compatibility with existing evidence
  readers.

Verification:

```text
cmake --build build/vs-debug-ffmpeg --config Debug --target kivo_video_playback_shell kivo_playback_shell_qml_load_test kivo_playback_shell_interaction_test kivo_playback_shell_video_surface_geometry_test
ctest --test-dir build/vs-debug-ffmpeg -C Debug --output-on-failure -R "^(kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_shell_video_surface_geometry_test)$"
cmake --build build/vs-debug-ffmpeg --config Release --target kivo_video_playback_shell
build\vs-debug-ffmpeg\ui\Release\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/ui_scenegraph_surface_shelter_release_20260706 --duration-ms 10000 --continuous-close-ms 500 --media "<UNC Shelter 2026 directory>"
```

Result:

- Focused UI CTest: `3/3` passed.
- Release Shelter SMB evidence:
  `artifacts/p10/ui_scenegraph_surface_shelter_release_20260706/continuous_playback_evidence.json`.
- Evidence summary:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`,
  `audio_error=""`,
  `audio_output_started=true`,
  `decoded_frame_count=218`,
  `rendered_frame_count=218`,
  `d3d11_device_type=hardware`,
  `d3d11_uploaded=true`,
  `d3d11_frames_uploaded=218`,
  `d3d11_upload_failures=0`,
  `d3d11_upload_max_elapsed_ms=10`,
  `p5_continuous_d3d11_upload_connected=true`,
  `p5_presentation_path=qt_scenegraph_texture`,
  `p5_hardware_presenter_connected=false`,
  `p7_background_pump_packet_budget=8`,
  `p7_background_pump_max_elapsed_ms=65`,
  `surface_refresh_max_elapsed_ms=1`,
  `surface_paint_max_elapsed_ms=0`,
  `audio_product_option_count=1`,
  `subtitle_product_option_count=0`,
  `p8_pgs_bitmap_render_connected=false`,
  `p8_embedded_packet_ingest_connected=false`,
  and captured stdout/stderr were both 0 bytes.
- Screenshot pixel sampling confirmed the 5s and 10s captures are nonblank.

Current truthful limits:

- This is a Qt scene graph texture presentation path, not a native swapchain
  presenter.
- Hardware frame pacing, D3D texture sharing into the scene graph, advanced HDR,
  full P8 subtitle rendering, PGS bitmap rendering, and TrueHD passthrough
  remain unfinished.

## 2026-07-06 Update: Product Path Avoids Unused D3D11 Upload Work

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- The user reported several seconds of stutter/audio trouble around the 1:00
  Shelter position.
- Existing Shelter/DTS/TrueHD/PGS fixes were not reworked.

Correction:

- The current shell presentation path is `qt_scenegraph_texture`; it consumes
  CPU RGBA frames and is not yet a native D3D swapchain presenter.
- Continuous per-frame D3D11 upload was therefore doing extra video work that
  was not used by the visible presentation path.
- `LocalVideoD3d11Upload` now defaults to disabled unless
  `KIVO_P5_D3D11_UPLOAD=1` is set.
- This keeps the explicit D3D11 upload proof path available, while the product
  playback path avoids unused upload cost until a native presenter consumes it.

Verification:

```text
cmake --build build\vs-debug-ffmpeg --config Release --target kivo_video_playback_shell
build\vs-debug-ffmpeg\ui\Release\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/shelter_product_no_unused_d3d11_upload_60s_release_20260706 --duration-ms 15000 --continuous-close-ms 200 --media "<UNC Shelter 2026 directory>"
KIVO_P5_D3D11_UPLOAD=1 build\vs-debug-ffmpeg\ui\Release\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/shelter_d3d11_upload_env_enabled_60s_release_20260706 --duration-ms 10000 --continuous-close-ms 200 --media "<UNC Shelter 2026 directory>"
cmake --build build\vs-debug-ffmpeg --config Debug --target kivo_playback_status_chrome_visibility_test kivo_playback_shell_qml_load_test kivo_playback_shell_interaction_test
ctest --test-dir build\vs-debug-ffmpeg -C Debug --output-on-failure -R "^(kivo_playback_status_chrome_visibility_test|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test)$"
```

Result:

- Product default Shelter evidence:
  `artifacts/p10/shelter_product_no_unused_d3d11_upload_60s_release_20260706/continuous_playback_evidence.json`.
- Product default key fields:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `audio_codec=dts`,
  `audio_error=""`,
  `decoded_frame_count=318`,
  `rendered_frame_count=318`,
  `audio_frames_written=668672`,
  `p7_background_pump_max_elapsed_ms=68`,
  `p7_background_pump_total_elapsed_ms=2616`,
  `surface_refresh_success_count=72`,
  `p5_presentation_path=qt_scenegraph_texture`,
  `p5_hardware_presenter_connected=false`,
  `p5_continuous_d3d11_upload_connected=false`.
- Explicit upload proof evidence:
  `artifacts/p10/shelter_d3d11_upload_env_enabled_60s_release_20260706/continuous_playback_evidence.json`.
- Explicit upload key fields:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `d3d11_uploaded=true`,
  `d3d11_frames_uploaded=202`,
  `p5_continuous_d3d11_upload_connected=true`,
  `p5_hardware_presenter_connected=false`.
- Focused UI CTest: `3/3` passed.

Truthful limit:

- This is a product-path performance correction and a truthful evidence split.
  It is not native D3D presentation, not hardware frame pacing, and not full P5
  presenter completion.

## 2026-07-06 Update: Shelter Seek Preroll Clock Fix

Scope:

- Local only. No push, no reset, no clean, no stash, no checkout/switch.
- The user reported a multi-second stall plus audio trouble around the Shelter
  `00:01:00` position.
- Existing Shelter DTS fallback, TrueHD fail-closed behavior, PGS hiding, and
  product default D3D11-upload-off behavior were not reworked.

Finding:

- Previous continuous evidence could report `PASS_CONTINUOUS_PLAYBACK` even when
  `seek_requested_ms=60000` and the final visible frame media time was still
  before the target.
- Repro baseline:
  `artifacts/p10/shelter_frame_time_evidence_60s_release_20260706/continuous_playback_evidence.json`
  showed `timeline_position_after_ms=66303` but
  `last_frame_time_us=54638000`, so P7/P4 believed playback had passed 60s
  while P5 was still presenting roughly 54.6s.

Correction:

- P5 local video runtime now keeps a seek preroll floor and suppresses audio
  packets, subtitle packets, and presented video frames whose media timestamps
  are before the seek/start target.
- P7 continuous playback now anchors its public timeline to the media time of
  the last presented video frame, and holds the target position while waiting
  for the first post-seek frame instead of letting wall-clock time drift ahead.
- P7 background pump keeps the normal packet budget at `8`, but temporarily
  uses a seek-only preroll budget of `256` while waiting for the first post-seek
  frame, then returns to the normal budget.
- Shell continuous evidence now requires the post-seek video frame media time
  to be at or beyond the requested seek target, and writes
  `frame_timeline_drift_ms`.

Verification:

```text
cmake --build build\vs-debug-ffmpeg --config Debug --target kivo_video_playback_graph kivo_video_player_runtime_adapter kivo_video_playback_shell
ctest --test-dir build\vs-debug-ffmpeg -C Debug --output-on-failure -R "^(kivo_player_runtime_adapter_tests|kivo_playback_shell_qml_load_test|kivo_playback_shell_interaction_test|kivo_playback_status_chrome_visibility_test)$"
cmake --build build\vs-debug-ffmpeg --config Release --target kivo_video_playback_shell
build\vs-debug-ffmpeg\ui\Release\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts/p10/shelter_seek_preroll_boost_final_release_20260706 --duration-ms 15000 --continuous-close-ms 200 --media "<UNC Shelter 2026 directory>"
```

Result:

- Final evidence:
  `artifacts/p10/shelter_seek_preroll_boost_final_release_20260706/continuous_playback_evidence.json`.
- Key fields:
  `classification=PASS_CONTINUOUS_PLAYBACK`,
  `errors=""`,
  `seek_requested_ms=60000`,
  `post_seek_frame_rendered=true`,
  `timeline_position_after_ms=63554`,
  `last_frame_time_us=63480000`,
  `frame_timeline_drift_ms=74`,
  `audio_codec=dts`,
  `audio_error=""`,
  `audio_frames_written=597344`,
  `audio_flush_count=1`,
  `audio_writer_reset_count=1`,
  `p7_background_pump_max_elapsed_ms=76`,
  `p7_background_pump_packet_budget=8`,
  `d3d11_uploaded=false`,
  `p5_presentation_path=qt_scenegraph_texture`,
  `p5_hardware_presenter_connected=false`.
- Focused Debug CTest: `4/4` passed.

Truthful limit:

- This fixes seek preroll honesty, stale pre-target audio/video presentation,
  and P7/P5 timeline drift around the Shelter 60s seek path.
- It is still not a native hardware-presented frame queue, not full audio-clock
  master synchronization, not TrueHD passthrough, and not full P8 subtitle
  rendering.
