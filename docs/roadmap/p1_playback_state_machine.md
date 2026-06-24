# P1 Playback State Machine

`FakePlaybackStateMachine` proves the first PlaybackCore invariants before real playback exists.

## Covered Transitions

- Idle to Ready through `open`.
- Ready to Playing through graph-bound `play`.
- Playing to Paused.
- Paused or Playing to Seeking with seek generation.
- Seeking back to Playing or Ready.
- Active session to Recovering with recovery action ref.
- Any non-closed state to Closed.

## Invariants

The state machine uses `check_playback_invariants` so P1 tests prove:

- Playing requires active session.
- Playing requires graph binding.
- Seeking requires seek generation.
- Recovering requires recovery action ref.
- Closed must not emit later playback events.

