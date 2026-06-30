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
build\vs-debug-ffmpeg\ui\Debug\kivo_video_playback_shell.exe --continuous-playback-evidence artifacts\p10\continuous --media "\\192.168.5.1\视频\电影\Shelter 2026 USA BluRay 1080p TrueHD5.1 x265 10bit-CHD" --duration-ms 10000
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
