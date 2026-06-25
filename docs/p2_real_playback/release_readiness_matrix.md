# P2-054 Release Readiness Matrix

## Purpose

Defines contracts for tracking release readiness across all P2 subsystems. Each readiness check evaluates a specific area (build, test, governance, documentation) and the matrix aggregates them into a release decision.

## Family Tree

```
release_core/
├── readiness_check.hpp          — Individual readiness check descriptor
├── readiness_matrix.hpp         — Aggregate matrix across all checks
├── release_gate.hpp             — Gate decision with blocking/non-blocking items
├── fake_release_readiness_service.hpp — Fake service with 12 builders
```

## Contract Types

### ReadinessCheck

Describes a single readiness check evaluation.

| Field | Type | Description |
|-------|------|-------------|
| check_id | string | Unique check identifier |
| check_area | string | Area being checked (build, test, governance, docs, integration) |
| check_status | enum | Pass / Fail / Warning / Skipped |
| blocking | bool | Whether failure blocks release |
| evidence | string | Evidence of check result |
| last_verified_iso | string | ISO 8601 timestamp of last verification |

### ReadinessMatrix

Aggregates all readiness checks into a release decision.

| Field | Type | Description |
|-------|------|-------------|
| matrix_id | string | Unique matrix identifier |
| version | string | Version being evaluated |
| total_checks | int | Total number of checks |
| passed_checks | int | Number of passed checks |
| failed_checks | int | Number of failed checks |
| warning_checks | int | Number of warning checks |
| blocking_failures | int | Number of blocking failures |
| release_ready | bool | Whether release is ready |

### ReleaseGate

Final gate decision for release.

| Field | Type | Description |
|-------|------|-------------|
| gate_id | string | Unique gate identifier |
| gate_decision | enum | Ready / NotReady / ConditionalReady |
| blocking_items | int | Count of blocking items |
| non_blocking_warnings | int | Count of non-blocking warnings |
| gate_message | string | Human-readable gate message |
| next_action | string | Recommended next action if not ready |

## FakeService: FakeReleaseReadinessService

12 static builder methods:

1. `build_passing_check()` — Check that passed
2. `build_failing_check()` — Check that failed (blocking)
3. `build_warning_check()` — Check with warning
4. `build_skipped_check()` — Check that was skipped
5. `build_ready_matrix()` — All checks pass
6. `build_not_ready_matrix()` — Blocking failures exist
7. `build_conditional_matrix()` — Non-blocking warnings only
8. `build_ready_gate()` — Gate decision: Ready
9. `build_not_ready_gate()` — Gate decision: NotReady
10. `build_conditional_gate()` — Gate decision: ConditionalReady
11. `build_build_area_checks()` — Build area checks
12. `build_governance_area_checks()` — Governance area checks

## Test Coverage

12 CTest tests covering:
- Readiness check construction and validation
- Readiness matrix aggregation
- Release gate decision logic
- Blocking vs non-blocking distinction
- Builder method correctness
- Area-specific check grouping
