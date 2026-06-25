# P2-053 Recovery / Fallback Runtime

## Purpose

Defines contracts for playback recovery actions, fallback strategies when primary paths fail, and failure budget tracking that prevents cascading failures across the playback pipeline.

## Family Tree

```
recovery_core/
├── recovery_action.hpp          — Individual recovery action descriptor
├── fallback_strategy.hpp        — Fallback plan when primary path fails
├── failure_budget.hpp           — Failure rate budget per session/component
├── fake_recovery_fallback_service.hpp — Fake service with 12 builders
```

## Contract Types

### RecoveryAction

Describes a single recovery action taken when a failure occurs.

| Field | Type | Description |
|-------|------|-------------|
| action_id | string | Unique action identifier |
| action_kind | enum | Retry / Fallback / Abort / RestartComponent / SkipTrack |
| trigger_error_domain | string | Error domain that triggered recovery |
| trigger_error_severity | enum | Warning / Error / Fatal |
| max_retries | int | Maximum retry attempts (0 = no retry) |
| retry_delay_ms | int64 | Delay between retries |
| user_visible_hint | string | User-facing explanation |

### FallbackStrategy

Describes a fallback plan when the primary playback path is unavailable.

| Field | Type | Description |
|-------|------|-------------|
| strategy_id | string | Unique strategy identifier |
| primary_path | string | Primary path description |
| fallback_path | string | Fallback path description |
| fallback_reason | enum | SourceUnavailable / CodecUnsupported / DeviceLost / NetworkTimeout / InsufficientBandwidth |
| estimated_quality_impact | enum | None / Minor / Moderate / Significant |
| auto_apply | bool | Whether fallback is applied automatically |
| user_notification_required | bool | Whether user must be notified |

### FailureBudget

Tracks failure rate budgets per session to prevent cascading failures.

| Field | Type | Description |
|-------|------|-------------|
| budget_id | string | Unique budget identifier |
| component | string | Component scope (demux, decode, render, audio) |
| window_size_seconds | int64 | Rolling window for failure rate calculation |
| max_failures_per_window | int | Maximum failures allowed in window |
| current_failures | int | Current failure count in window |
| budget_exhausted | bool | Whether budget is currently exhausted |
| recovery_cooldown_ms | int64 | Cooldown after budget exhaustion |

## FakeService: FakeRecoveryFallbackService

12 static builder methods:

1. `build_retry_action()` — Retry with exponential backoff
2. `build_fallback_action()` — Fallback to software decode
3. `build_abort_action()` — Abort playback with error
4. `build_restart_action()` — Restart component
5. `build_source_unavailable_fallback()` — Fallback when source unavailable
6. `build_codec_unsupported_fallback()` — Fallback when codec unsupported
7. `build_network_timeout_fallback()` — Fallback when network timeout
8. `build_green_budget()` — Budget with no failures
9. `build_warning_budget()` — Budget near limit
10. `build_exhausted_budget()` — Budget fully exhausted
11. `build_recovering_budget()` — Budget in cooldown recovery
12. `build_cross_component_budget()` — Budget spanning demux+decode

## Test Coverage

12 CTest tests covering:
- Recovery action construction and validation
- Fallback strategy quality impact levels
- Failure budget tracking and exhaustion
- Budget cooldown and recovery
- Cross-component budget interaction
- Builder method correctness
