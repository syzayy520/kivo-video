# P2-028 Network Security Policy Skeleton

Updated: 2026-06-25

## Goal

Establish network security policy contracts for redirect, TLS, SSRF, and credential handling. These policies protect against common remote playback security threats.

## Scope

Define security policies that govern how source adapters handle network connections, credentials, and redirects. All policies are fail-closed by default.

## Contract Types

### CredentialPolicy

Controls how credentials are stored, redacted, and injected into network requests.

```cpp
struct CredentialPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::string credential_storage;  // "memory_only", "encrypted_file", "system_credential_manager"
    bool redact_credentials_in_logs{true};
    bool redact_credentials_in_inspector{true};
    bool allow_credential_injection{true};
    std::string injection_method;  // "header", "query_param", "basic_auth"
    std::string credential_rotation_policy;  // "never", "session_bound", "time_bound"
};
```

### TlsPolicy

Controls TLS connection requirements for source adapters.

```cpp
struct TlsPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    std::string min_tls_version;  // "1.2", "1.3"
    bool verify_certificates{true};
    bool allow_self_signed{false};
    bool enable_certificate_pinning{false};
    std::string tls_fallback;  // "none", "downgrade_to_http"
};
```

### SsrfPolicy

Protects against Server-Side Request Forgery attacks.

```cpp
struct SsrfPolicy {
    ContractMetadata metadata;
    std::string policy_id;
    bool block_private_ips{true};
    bool block_localhost{true};
    bool block_metadata_endpoints{true};
    std::vector<std::string> allowed_hosts;
};
```

## Fake Service

Provides deterministic security policy results for testing:

- `build_strict_credential_policy()` - Redacts credentials everywhere
- `build_relaxed_credential_policy()` - Allows credential injection
- `build_strict_tls_policy()` - TLS 1.3, cert pinning
- `build_permissive_tls_policy()` - Allows self-signed certs
- `build_strict_ssrf_policy()` - Blocks private IPs, localhost, metadata endpoints
- `build_permissive_ssrf_policy()` - Allows specific hosts only

## Tests

1. Strict credential redaction policy
2. Relaxed credential injection policy
3. Strict TLS policy with cert pinning
4. Permissive TLS policy
5. Strict SSRF policy blocking private IPs
6. Permissive SSRF policy with allowed hosts

## Evidence Target

redirect, TLS, SSRF, and credential policy docs/tests.
