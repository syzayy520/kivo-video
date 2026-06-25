# P2-046 Crash Dump / Support Bundle Privacy Policy

## Purpose

Defines crash dump and support bundle privacy policy, including fail-closed redaction rules, data minimization, and user consent requirements.

## Scope

- Crash dump privacy rules
- Support bundle redaction policies
- Fail-closed privacy guarantees
- User consent and transparency
- Data minimization requirements

## Family Tree (diagnostic_core/)

```
diagnostic_core/
├── crash_dump_privacy_policy.hpp       — Crash dump privacy configuration
├── support_bundle_redaction_policy.hpp — Support bundle redaction rules
├── privacy_consent_record.hpp          — User consent tracking
├── fake_crash_dump_privacy_service.hpp — Fake service for testing
```

## Contract Types

### CrashDumpPrivacyPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| collect_crash_dumps | bool | Whether crash dumps are collected |
| redact_pii | bool | Whether PII is redacted |
| redact_credentials | bool | Whether credentials are redacted |
| fail_closed_on_redaction_failure | bool | Whether to fail closed if redaction fails |
| retention_days | int | Crash dump retention period |
| require_user_consent | bool | Whether user consent is required |

### SupportBundleRedactionPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| redact_user_paths | bool | Whether user file paths are redacted |
| redact_network_info | bool | Whether network information is redacted |
| redact_memory_contents | bool | Whether memory contents are redacted |
| redact_clipboard_data | bool | Whether clipboard data is redacted |
| redaction_level | string | Redaction level (none, minimal, standard, maximum) |
| fail_closed_on_redaction_failure | bool | Whether to fail closed if redaction fails |

### PrivacyConsentRecord

| Field | Type | Description |
|-------|------|-------------|
| record_id | string | Unique record identifier |
| user_id | string | User identifier |
| consent_given | bool | Whether consent was given |
| consent_timestamp | int64_t | When consent was given |
| consent_scope | string | Scope of consent (crash_dumps, support_bundles, both) |
| consent_version | string | Version of consent policy |
| can_revoke | bool | Whether consent can be revoked |

## Tests

10 tests covering:
1. CrashDumpPrivacyPolicy construction with default values
2. SupportBundleRedactionPolicy construction with standard redaction
3. PrivacyConsentRecord construction with consent given
4. Fail-closed policy on redaction failure
5. Fail-open policy on redaction failure
6. Maximum redaction level policy
7. Minimal redaction level policy
8. User consent required policy
9. User consent not required policy
10. Consent revocation policy