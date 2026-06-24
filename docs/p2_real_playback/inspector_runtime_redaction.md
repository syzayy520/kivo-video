# P2-021 Minimal Inspector / Diagnostic Redaction

Updated: 2026-06-25

## Goal

Implement Inspector runtime for real playback path and redaction verification.

## Inspector Runtime Contract

Already defined in `playback_inspector_core/inspector_runtime.hpp`:

```cpp
struct InspectorRuntimeContract {
    ContractMetadata metadata;
    std::string inspector_id;
    bool reads_decision_ledger{true};
    bool reads_telemetry_timeline{true};
    bool reads_capability_join{true};
    bool supports_redaction{true};
};
```

## Fake Inspector Runtime Service

Provides deterministic inspector runtime configurations for testing:

- `build_default_inspector_runtime()` - all data sources enabled, redaction supported
- `build_decision_ledger_only_inspector()` - only decision ledger enabled
- `build_telemetry_only_inspector()` - only telemetry timeline enabled
- `build_no_redaction_inspector()` - redaction disabled
- `build_disabled_inspector()` - all data sources disabled

## Redaction Verification

Uses existing `DiagnosticRedactionGate` to verify redaction works with inspector data:

- Test payload with token, cookie, signed_url, credential secrets
- Verify secrets are redacted
- Verify redacted markers are present

## Tests

1. Default inspector runtime
2. Decision ledger only inspector
3. Telemetry only inspector
4. No redaction inspector
5. Disabled inspector
6. Redaction with inspector data

## Evidence Target

Inspector runtime for real playback path and redaction verification.