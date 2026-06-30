# PlayerRuntimeAdapter Natural Family Tree

## Boundary

QML `PlayerPage` talks to one object: `PlayerRuntimeAdapter`.
`PlayerRuntimeAdapter` talks to one backend owner: P7 `PlaybackSession`.
P7 remains the only route to P4/P5/P6/P8.

Forbidden from this family: direct QML access to FFmpeg, WASAPI, P4, P5, P6, P8,
decode queues, device threads, generation stamps, epochs, or demux/runtime internals.

## Family

1. `include/kivo/video/player_runtime_adapter`
   Public adapter contract, command results, snapshot fields, and UI-facing value types.

2. `backend/src/video/player_runtime_adapter/core`
   Owns the adapter object and lifecycle. It only calls the P7 bridge and state mapper.

3. `backend/src/video/player_runtime_adapter/p7_bridge`
   Converts adapter commands to P7 `PlaybackSession` calls. Missing P7 APIs return typed
   `NotConnectedToP7`; they are not faked in UI state.

4. `backend/src/video/player_runtime_adapter/state`
   Maps P7 snapshots to adapter snapshot fields such as opening, buffering, paused,
   playback state, error, and connection status.

5. `ui/playback/page`
   QML view composition only. User actions are dispatched to `runtimeAdapter`; QML does
   not maintain true playback, seek, volume, track, or fullscreen backend state.

## Required Connection Checklist

1. Video surface: `attach_video_surface`, `release_surface` call P7 binding APIs; telemetry
   fields come from `query_video_surface`.
2. Playback controls: open, play, pause, toggle, seek, audio/video track switch, close,
   reopen, and release surface call P7 public APIs.
3. Timeline: position, duration, and buffered ranges come from P7 `query_timeline` and queue
   snapshots; adapter marks timeline/buffered connections `ConnectedToP7` when valid.
4. Subtitle tracks/delay: `select_subtitle_track`, `disable_subtitle`, and `set_subtitle_delay`
   call P7 coordination commands. `subtitle_frame` stays `NotConnectedToP7` until P7 exposes a
   real P8-backed frame lease; adapter does not fabricate cue text.
5. Audio volume/mute/device/delay: adapter commands call P7 `SetAudioOutputPolicy` fake contract;
   this is not a real P6/WASAPI bridge.
6. Playback settings policy: aspect/scale/tone/deinterlace/speed/subtitle-size call P7 policy
   commands and appear in adapter snapshot when valid.
7. Error/recovery/evidence: adapter reflects P7 snapshot error state; `reopen` and
   `copy_diagnostics` use P7 recovery/evidence queries. `retry` requires Failed state.
8. Stop transport: `stop()` calls P7 `Stop` and returns session to `Ready` without `close()`.
9. Open/close lifecycle: `open`, `close`, and `releaseSurface` are adapter-owned entry
   points; page exit must call close and release surface through the adapter.
10. Buffering/loading/rebuffer: adapter maps P7 graph states into opening/buffering fields.
11. Playlist/next episode: P7 closure excludes playlist orchestration; adapter keeps typed
    `NotConnectedToP7`.
12. Fullscreen/screen policy: UI/window concern; adapter keeps typed `NotConnectedToP7`.
13. Track inventory cycle shortcuts: no P7 public inventory yet; audio cycle and subtitle
    enable-cycle remain `NotConnectedToP7` except subtitle disable when already enabled.
14. Input shortcuts: QML sends shortcut intent to adapter. Adapter dispatches through P7-backed
    commands or returns typed `NotConnectedToP7`.
