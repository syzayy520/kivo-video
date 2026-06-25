# P2-042 Playback Inspector Runtime

## Purpose

Full Playback Inspector runtime that reads snapshots, decision ledger, and telemetry data to produce diagnostic reports. Enables post-mortem analysis of playback sessions.

## Scope

- Inspector read requests (snapshots, ledger entries, telemetry)
- Inspector read results (formatted data)
- Inspector summary reports
- Inspector runtime configuration

## Family Tree (playback_inspector_core/)

```
playback_inspector_core/
├── inspector_runtime.hpp                    — Runtime contract (existing)
├── inspector_read_request.hpp               — Read request
├── inspector_read_result.hpp                — Read result
├── fake_inspector_runtime_service.hpp       — Fake service for testing (expanded)
```

## Contract Types

### InspectorReadRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique request identifier |
| inspector_id | string | Inspector requesting data |
| source_type | string | "snapshot", "ledger", "telemetry" |
| session_id | string | Playback session to inspect |
| include_redacted | bool | Whether to include redacted fields |

### InspectorReadResult

| Field | Type | Description |
|-------|------|-------------|
| result_id | string | Unique result identifier |
| request_id | string | Reference to request |
| source_type | string | Type of data returned |
| data_payload | string | Serialized inspection data |
| record_count | int64_t | Number of records returned |
| was_redacted | bool | Whether redaction was applied |

## Tests

10 tests covering:
1. Read snapshot request construction
2. Read ledger request construction
3. Read telemetry request construction
4. Successful read result
5. Empty session read result
6. Redacted read result
7. Inspector summary generation
8. Default inspector runtime (reads all sources)
9. Decision-ledger-only inspector
10. Disabled inspector
