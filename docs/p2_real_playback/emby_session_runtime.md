# P2-036 Emby Session Runtime

## Purpose

Model the Emby media server session lifecycle: creating sessions, binding to Emby items, tracking playback state, and handling session teardown.

## Scope

- Emby session identity and item binding
- Session lifecycle (create, play, pause, stop, teardown)
- Session failure handling (expired, revoked, server unreachable)

## Family Tree (source_core/)

```
source_core/
├── emby_session_identity.hpp      — Stable identity for an Emby session
├── emby_session_runtime.hpp       — Runtime lifecycle and state
├── emby_session_failure.hpp       — Failure kinds for session operations
├── fake_emby_session_service.hpp  — Fake service for testing
```

## Contract Types

### EmbySessionIdentity

| Field | Type | Description |
|-------|------|-------------|
| session_id | string | Unique session identifier |
| server_id | string | Emby server identifier |
| item_id | string | Emby media item identifier |
| media_source_id | string | Media source within the item |
| user_id | string | Emby user who initiated the session |
| device_id | string | Client device identifier |

### EmbySessionRuntime

| Field | Type | Description |
|-------|------|-------------|
| session_id | string | Correlated session ID |
| state | EmbySessionState | Current session state |
| created_at | int64_t | Session creation timestamp |
| last_activity_at | int64_t | Last activity timestamp |
| playback_position_ticks | int64_t | Current position in ticks (10 million = 1 sec) |
| is_paused | bool | Whether playback is paused |

### EmbySessionState

| Value | Description |
|-------|-------------|
| Created | Session created, not yet playing |
| Playing | Active playback |
| Paused | Playback paused |
| Stopped | Playback stopped |
| TornDown | Session cleaned up |

### EmbySessionFailure

| Field | Type | Description |
|-------|------|-------------|
| failure_kind | EmbySessionFailureKind | Kind of failure |
| failure_reason | string | Human-readable reason |
| retry_after_ms | int64_t | Retry delay (0 = no retry) |
| is_recoverable | bool | Whether retry may succeed |

### EmbySessionFailureKind

| Value | Description |
|-------|-------------|
| SessionExpired | Session token expired |
| ServerUnreachable | Emby server unreachable |
| ItemNotFound | Media item not found on server |
| AccessDenied | User lacks permission |

## Tests

10 tests in `backend/tests/source_core/emby_session_test.cpp`:
1. Session identity construction
2. Session runtime lifecycle
3. Session state transitions
4. Playback position tracking
5. Pause/resume tracking
6. Session teardown
7. Expired session failure
8. Server unreachable failure
9. Item not found failure
10. Access denied failure
