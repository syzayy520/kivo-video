# P2-052 Build / Signing / Symbol / Update Policy Skeleton

## Purpose

Defines build pipeline security, code signing, symbol management, and application update policy contracts for safe and reproducible builds.

## Scope

- Build policy and configuration
- Code signing requirements
- Symbol management policy
- Application update policy
- Build reproducibility

## Family Tree

```
backend/include/kivo/cinema_engine/
├── build_core/
│   ├── build_config_policy.hpp           — Build configuration policy
│   └── fake_build_config_service.hpp     — Fake service for testing
├── signing_core/
│   ├── code_signing_policy.hpp           — Code signing policy
│   └── fake_signing_service.hpp          — Fake service for testing
├── symbol_core/
│   ├── symbol_management_policy.hpp      — Symbol management policy
│   └── fake_symbol_service.hpp           — Fake service for testing
└── update_core/
    ├── update_policy.hpp                 — Application update policy
    └── fake_update_service.hpp           — Fake service for testing
```

## Contract Types

### BuildConfigPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| build_type | string | Build type (Release, Debug, RelWithDebInfo) |
| reproducible_build | bool | Whether build must be reproducible |
| deterministic_timestamps | bool | Whether timestamps are deterministic |
| strip_debug_info | bool | Whether to strip debug info |
| max_build_time_seconds | int | Maximum allowed build time |

### CodeSigningPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| signing_required | bool | Whether code signing is required |
| certificate_validation | bool | Whether to validate certificates |
| timestamp_signing | bool | Whether to use timestamp signing |
| allowed_signers | list | List of allowed signer identities |
| revocation_check | bool | Whether to check certificate revocation |

### SymbolManagementPolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| symbol_server_enabled | bool | Whether symbol server is enabled |
| symbol_upload_required | bool | Whether symbol upload is required |
| retention_days | int | Symbol retention period |
| compression_enabled | bool | Whether to compress symbols |
| public_symbols_only | bool | Whether to publish only public symbols |

### UpdatePolicy

| Field | Type | Description |
|-------|------|-------------|
| policy_id | string | Unique policy identifier |
| auto_update_enabled | bool | Whether auto-update is enabled |
| signature_verification | bool | Whether to verify update signatures |
| rollback_enabled | bool | Whether rollback is supported |
| max_update_size_mb | int | Maximum update package size |
| staged_rollout | bool | Whether to use staged rollout |

## Fake Services

4 fake services, 12 builder methods total:

### FakeBuildConfigService (3 methods)
- build_release_build_config()
- build_debug_build_config()
- build_reproducible_build_config()

### FakeSigningService (3 methods)
- build_strict_signing_policy()
- build_relaxed_signing_policy()
- build_no_signing_policy()

### FakeSymbolService (3 methods)
- build_full_symbol_policy()
- build_public_only_symbol_policy()
- build_minimal_symbol_policy()

### FakeUpdateService (3 methods)
- build_strict_update_policy()
- build_staged_update_policy()
- build_manual_update_policy()

## Tests

12 tests covering:
1. Release build config construction
2. Debug build config construction
3. Reproducible build config construction
4. Strict signing policy construction
5. Relaxed signing policy construction
6. No signing policy construction
7. Full symbol policy construction
8. Public only symbol policy construction
9. Minimal symbol policy construction
10. Strict update policy construction
11. Staged update policy construction
12. Manual update policy construction
