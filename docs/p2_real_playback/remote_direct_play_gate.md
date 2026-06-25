# P2-038 Remote Direct Play Gate

## Purpose

Define the safety gate for remote direct play: ensuring no silent transcoding occurs without user consent, credentials are redacted from logs, redirect URLs do not leak credentials, and cache entries bind to the correct remote object identity.

## Scope

- No-silent-transcode gate (user must approve transcoding)
- Credential redaction in logs and error messages
- Redirect leakage prevention (no credentials in redirect URLs)
- Cache identity binding (cache entries tied to remote object identity)

## Family Tree (network_cache_core/)

```
network_cache_core/
├── remote_direct_play_gate_request.hpp      — Gate check request
├── remote_direct_play_gate_result.hpp       — Gate check result
├── remote_direct_play_gate_policy.hpp       — Gate policy configuration
├── fake_remote_direct_play_gate_service.hpp — Fake service for testing
```

## Contract Types

### RemoteDirectPlayGateRequest

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Unique identifier |
| source_url | string | Original source URL |
| is_direct_play | bool | true = direct play, false = transcoding |
| has_credential | bool | Whether URL contains credentials |
| redirect_chain | vector<string> | Redirect URLs encountered |
| cache_identity_id | string | Expected cache identity binding |

### RemoteDirectPlayGateResult

| Field | Type | Description |
|-------|------|-------------|
| request_id | string | Correlated request ID |
| gate_passed | bool | Whether the gate check passed |
| transcode_requires_approval | bool | true if transcoding needs user approval |
| credentials_redacted | bool | true if credentials were redacted |
| redirect_leak_detected | bool | true if redirect contains credentials |
| cache_binding_valid | bool | true if cache identity matches |
| failure_reason | string | Reason if gate failed |

### RemoteDirectPlayGatePolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Policy identifier |
| block_silent_transcode | bool | Block transcoding without approval |
| redact_credentials_in_logs | bool | Redact credentials in log output |
| block_credential_in_redirect | bool | Block redirects containing credentials |
| require_cache_identity_binding | bool | Require cache identity binding |

## Tests

10 tests in `backend/tests/network_cache_core/remote_direct_play_gate_test.cpp`:
1. Direct play passes gate
2. Transcode requires approval
3. Credential redaction in request
4. Redirect with credential detected
5. Redirect without credential passes
6. Cache identity binding valid
7. Cache identity binding mismatch
8. Policy with all checks enabled
9. Policy with checks disabled
10. Gate failure reason reporting
