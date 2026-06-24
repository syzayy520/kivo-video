# P2-009 Playback Command Contract

Updated: 2026-06-25

## Goal

Implement generation-token command handling for playback commands with monotonic generation tracking.

## Command Types

| Command | Payload | Generation |
|---------|---------|------------|
| Open | session_id | 1 |
| Play | — | increment |
| Pause | — | increment |
| Seek | seek_target_us | increment |
| Recover | recovery_action_ref | increment |
| Close | — | final |

## Generation Token Rules

1. Generation starts at 1 on Open
2. Each command increments generation
3. Commands with stale generation are rejected
4. Close is always accepted regardless of generation

## Implementation Plan

1. Create `FakePlaybackCommandService` that produces commands with generation tracking
2. Tests covering command creation, generation increment, and stale rejection
3. Design doc
