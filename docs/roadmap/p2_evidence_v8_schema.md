# P2 Evidence V8 Schema

Updated: 2026-06-26

Canonical schema name: `p2-evidence-v8`.

This document defines the required fields for all V8 evidence JSON files under `artifacts/p2/evidence/`.

## Schema Fields

### Required Top-Level Fields

| Field | Type | Description |
|-------|------|-------------|
| `schema_version` | string | Must be `"p2-evidence-v8"`. |
| `task_id` | string | Task identifier (e.g., `"V8-001"`, `"LRD-013"`, `"SGF-001"`). |
| `task_name` | string | Human-readable task name. |
| `family` | string | Task family (e.g., `"governance"`, `"local_repair_delta/evidence"`). |
| `status` | string | Evidence status: one of `RUNTIME_PASS`, `CONTRACT_PASS`, `FAIL`, `BLOCKED_ENV`, `NOT_IMPLEMENTED`, `SKIPPED_TEST_ONLY`. |
| `runtime_mode` | string | Runtime mode: `"runtime"`, `"contract"`, `"audit"`, `"regression"`, `"classification"`, `"schema"`, `"redline"`. |
| `runtime_environment_kind` | string | Environment kind: `"test_harness"`, `"local_machine"`, `"remote_server"`, `"cloud_account"`, `"simulated"`, `"none"`. |
| `environment_required` | boolean | Whether real environment is required for RUNTIME_PASS. |
| `environment_available` | boolean | Whether real environment is available. |
| `blocked_reason` | string or null | Reason if blocked; null if not blocked. |
| `runtime_verified_timestamp` | string (ISO 8601) | Timestamp when runtime verification occurred. |
| `evidence_file` | string | Path to this evidence file (relative to workspace root). |
| `provider_matrix_impact` | object | Impact on provider runtime matrix (key-value pairs). |

### Optional Fields (Depending on Task Family)

#### Media/Source/Extension Identities
```json
{
  "media_identity": {
    "sample_id": "string",
    "file_path": "string (redacted)",
    "duration_seconds": "number",
    "video_codec": "string",
    "audio_codec": "string",
    "video_dimensions": "string",
    "audio_sample_rate": "number",
    "audio_channels": "number"
  },
  "source_identity": {
    "source_kind": "string",
    "protocol": "string",
    "host": "string (redacted)"
  },
  "extension_identity": {
    "extension_kind": "string",
    "extension_id": "string"
  }
}
```

#### Inspector Snapshot
```json
{
  "inspector_snapshot": {
    "source_info": "object",
    "probe_info": "object",
    "demux_info": "object",
    "decode_info": "object",
    "presenter_info": "object",
    "audio_info": "object",
    "clock_info": "object",
    "seek_info": "object",
    "lifecycle_info": "object",
    "threading_info": "object",
    "errors": "array"
  }
}
```

#### DecisionLedger
```json
{
  "decision_ledger": {
    "entries": "array of decision entries",
    "monotonic_ordering": "boolean",
    "total_entries": "number"
  }
}
```

#### Error Information
```json
{
  "errors": {
    "total_errors": "number",
    "error_domains": "array of strings",
    "error_severities": "array of strings",
    "recovery_actions": "array of strings"
  }
}
```

#### Redaction Information
```json
{
  "redaction": {
    "credential_redacted": "boolean",
    "path_redacted": "boolean",
    "token_redacted": "boolean",
    "privacy_compliance": "boolean"
  }
}
```

#### Schema Validation
```json
{
  "schema_validation": {
    "required_fields_present": "boolean",
    "status_vocabulary_compliance": "boolean",
    "anti_fake_compliance": "boolean",
    "redline_compliance": "boolean"
  }
}
```

#### Anti-fake Assertions
```json
{
  "anti_fake": {
    "unbounded_growth": "boolean",
    "repeated_open_close_fake": "boolean",
    "silent_fallback": "boolean",
    "evidence_contains_fake_as_real": "boolean",
    "stub_not_called_real_runtime": "boolean"
  }
}
```

#### Redline Information
```json
{
  "redline": {
    "forbidden_terms_scan": "array of strings",
    "premature_claim_detected": "boolean",
    "report_source_conflict": "boolean"
  }
}
```

#### Commands (Test Commands Used)
```json
{
  "commands": {
    "build_command": "string",
    "test_command": "string",
    "verification_command": "string"
  }
}
```

#### Artifacts (Generated Files)
```json
{
  "artifacts": {
    "total_artifacts": "number",
    "artifact_paths": "array of strings",
    "schema_validated_artifacts": "array of strings"
  }
}
```

## Example Evidence File

See `artifacts/p2/evidence/LRD-013.json` for a complete example.

## Validation Rules

1. `schema_version` must be `"p2-evidence-v8"`.
2. `status` must be one of the six allowed values.
3. `runtime_mode` must be one of the allowed modes.
4. `runtime_environment_kind` must be one of the allowed kinds.
5. `provider_matrix_impact` must be present (can be empty object).
6. All timestamps must be ISO 8601 format.
7. All file paths must be relative to workspace root.
8. No full paths, tokens, cookies, or credentials in any field.

## Schema Validation Command

SGF-006 will implement a validation command that checks all evidence files against this schema.

## Negative Tests

SGF-007 will implement negative tests to ensure:
- Simulator + RUNTIME_PASS fails
- unit_test + Runtime Final PASS fails
- credential_redacted=false + PASS fails
- CONTRACT_PASS + runtime_verified=true fails
- RUNTIME_PASS + runtime_verified=false fails
- mock runtime + RUNTIME_PASS fails