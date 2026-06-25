# P2 Decision Ledger V8 Schema

Updated: 2026-06-26

Canonical schema name: `p2-decision-ledger-v8`.

This document defines the required fields for Decision Ledger evidence in V8 evidence files.

## Decision Ledger Structure

The Decision Ledger is an optional field group in `p2-evidence-v8` schema. It provides a monotonic ordered list of decisions made during playback.

### Top-Level Decision Ledger Fields

| Field | Type | Description |
|-------|------|-------------|
| `entries` | array | List of decision entries in chronological order. |
| `monotonic_ordering` | boolean | Whether entries are in monotonic order (sequence numbers increasing). |
| `total_entries` | number | Total number of entries in the ledger. |
| `first_entry_sequence` | number | Sequence number of the first entry. |
| `last_entry_sequence` | number | Sequence number of the last entry. |
| `ledger_integrity` | boolean | Whether ledger integrity checks pass. |

### Decision Entry Fields

Each entry in the `entries` array must contain:

| Field | Type | Description |
|-------|------|-------------|
| `sequence_number` | number | Monotonically increasing sequence number. |
| `timestamp_ms` | number | Timestamp in milliseconds when decision was made. |
| `decision_type` | string | Type of decision (see Decision Type Vocabulary). |
| `decision_stage` | string | Pipeline stage where decision was made (see Stage Vocabulary). |
| `reason` | string | Human-readable reason for the decision. |
| `input` | object | Input data for the decision (varies by decision type). |
| `output` | object | Output/result of the decision (varies by decision type). |
| `generation` | number | Playback generation when decision was made. |
| `redaction_state` | object | Redaction information for this entry. |
| `provider_impact` | object | Impact on provider runtime matrix (if any). |
| `runtime_status` | string | Runtime status after this decision (from V8 status vocabulary). |
| `error_info` | object or null | Error information if decision failed; null if successful. |

### Decision Type Vocabulary

Allowed values for `decision_type`:

- `source_selection`: Source selection decision.
- `probe_decision`: FFmpeg probe decision.
- `demux_decision`: Demultiplexer decision.
- `decode_decision`: Decoder decision.
- `format_negotiation`: Format negotiation decision.
- `clock_sync`: Clock synchronization decision.
- `seek_decision`: Seek/flush decision.
- `pause_decision`: Pause decision.
- `resume_decision`: Resume decision.
- `stop_decision`: Stop decision.
- `close_decision`: Close decision.
- `error_recovery`: Error recovery decision.
- `quality_decision`: Quality/codec selection decision.
- `resource_allocation`: Resource allocation decision.
- `policy_enforcement`: Policy enforcement decision.

### Stage Vocabulary

Allowed values for `decision_stage`:

- `source`: Source stage.
- `probe`: Probe stage.
- `demux`: Demux stage.
- `decode`: Decode stage.
- `render`: Render stage.
- `audio`: Audio stage.
- `clock`: Clock stage.
- `seek`: Seek stage.
- `lifecycle`: Lifecycle stage.
- `threading`: Threading stage.
- `policy`: Policy stage.
- `recovery`: Recovery stage.

### Input/Output Structures

Input and output objects vary by decision type. Common fields:

#### Source Selection Input/Output
```json
{
  "input": {
    "available_sources": "array of source identities",
    "selection_criteria": "string (quality, compatibility, user_preference)"
  },
  "output": {
    "selected_source": "source identity",
    "selection_reason": "string"
  }
}
```

#### Probe Decision Input/Output
```json
{
  "input": {
    "source_path": "string (redacted)",
    "probe_timeout_ms": "number"
  },
  "output": {
    "probe_success": "boolean",
    "format": "string",
    "duration_seconds": "number",
    "streams_found": "number"
  }
}
```

#### Seek Decision Input/Output
```json
{
  "input": {
    "seek_target_ms": "number",
    "seek_type": "string (absolute, relative, relative_to_end)",
    "flush_scope": "string (demux, decoder, both, none)"
  },
  "output": {
    "seek_success": "boolean",
    "actual_position_ms": "number",
    "frames_dropped": "number",
    "generation_incremented": "boolean"
  }
}
```

### Redaction State Structure

```json
{
  "credential_redacted": "boolean",
  "path_redacted": "boolean",
  "token_redacted": "boolean",
  "privacy_compliance": "boolean"
}
```

### Provider Impact Structure

```json
{
  "provider_kind": "string",
  "impact_type": "string (none, positive, negative, neutral)",
  "impact_details": "string"
}
```

### Error Info Structure

```json
{
  "error_id": "string",
  "domain": "string",
  "severity": "string (warning, error, fatal)",
  "message": "string (redacted)",
  "recoverable": "boolean",
  "recovery_action": "string (retry, skip, abort, fallback)"
}
```

## Usage in Evidence Files

Decision Ledger is included in evidence files as an optional field group:

```json
{
  "schema_version": "p2-evidence-v8",
  "task_id": "LRD-008",
  "decision_ledger": {
    "entries": [
      {
        "sequence_number": 1,
        "timestamp_ms": 1000,
        "decision_type": "source_selection",
        "decision_stage": "source",
        "reason": "Selected local file based on user preference",
        "input": { ... },
        "output": { ... },
        "generation": 1,
        "redaction_state": { ... },
        "provider_impact": { ... },
        "runtime_status": "CONTRACT_PASS",
        "error_info": null
      }
    ],
    "monotonic_ordering": true,
    "total_entries": 10,
    "first_entry_sequence": 1,
    "last_entry_sequence": 10,
    "ledger_integrity": true
  }
}
```

## Validation Rules

1. `entries` must be an array (can be empty).
2. `monotonic_ordering` must be true if entries exist.
3. `total_entries` must equal length of `entries` array.
4. Sequence numbers must be strictly increasing.
5. All timestamps must be in milliseconds.
6. All decision types and stages must be from allowed vocabularies.
7. All runtime statuses must be from V8 status vocabulary.
8. No full paths, tokens, or credentials in any field.

## Example Evidence Files

See `artifacts/p2/evidence/LRD-008.json` and `artifacts/p2/evidence/LRD-013.json` for examples.