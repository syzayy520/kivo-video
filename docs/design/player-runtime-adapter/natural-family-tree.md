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
4. Subtitle overlay and subtitle tracks: exposed by adapter contract; current P7 public API
   has no subtitle frame or subtitle track switch, so adapter reports `NotConnectedToP7`.
5. Audio volume/device: exposed by adapter contract; current P7 public API has no P6
   volume/device command, so adapter reports `NotConnectedToP7`.
6. Quality/HDR/settings: exposed by adapter contract; policy commands report
   `NotConnectedToP7` until P7 publishes them.
7. Error/recovery/evidence: adapter reflects P7 snapshot error state; `reopen` and
   `copy_diagnostics` use P7 recovery/evidence queries. `retry` requires Failed state.
8. Open/close lifecycle: `open`, `close`, and `releaseSurface` are adapter-owned entry
   points; page exit must call close and release surface through the adapter.
9. Fullscreen/window/screen: adapter exposes commands; current P7 public API has no screen
   or viewport policy notification, so adapter reports `NotConnectedToP7`.
10. Buffering/loading/rebuffer: adapter maps P7 graph states into opening/buffering fields.
11. Playlist/next episode: adapter exposes commands; current P7 scope has no playlist or
   EOS next orchestration, so adapter reports `NotConnectedToP7`.
12. Input shortcuts: QML sends shortcut intent to adapter. Adapter dispatches only through
   P7-backed commands or returns typed `NotConnectedToP7`.
13. User settings overlay: subtitle size/delay, audio delay, aspect, tone mapping, and
   speed are adapter commands; current P7 public API is missing the policy endpoints.
