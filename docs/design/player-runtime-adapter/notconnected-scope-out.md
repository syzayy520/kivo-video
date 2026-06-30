# Adapter NotConnected Scope-Out Registry

## P7_SCOPE_OUT_PLAYLIST

Playlist and EOS-next APIs remain typed `NotConnectedToP7`:

- `next()`
- `previous()`
- `play_next_episode()`
- `set_auto_play_next()`
- `hasNext` / `hasPrevious` / `nextItemTitle`

Defer to P10 playlist/session queue. Do not fake playlist state in P7 or QML.

## P7_SCOPE_OUT_WINDOW_POLICY

Fullscreen and screen policy remain typed `NotConnectedToP7`:

- `enter_fullscreen()`
- `exit_fullscreen()`
- `move_to_screen()`
- fullscreen shortcuts

Defer to UI/window policy layer. Do not force into P7 playback core.

## NotConnectedToP8Runtime

`subtitle_frame` reports `NotConnectedToP8Runtime` only when P7 has no active P8 runtime
attachment (subtitle disabled or bridge not started). When P8 is attached but no cue is active,
`subtitle_frame` stays `ConnectedToP7` with `subtitle_frame_available=false`.