# P2-027 Source Adapter Contract Skeleton

Updated: 2026-06-25

## Goal

Establish source adapter contract skeleton with identity, capability, and failure types for remote playback adapters (WebDAV, AList, Emby, local file).

## Scope

This task expands the source_core/ family with adapter-specific contracts that define how source adapters declare their identity, capabilities, and failure modes.

## Contract Types

### SourceAdapterIdentity

Adapter identification including protocol, host, and credential scope.

```cpp
struct SourceAdapterIdentity {
    ContractMetadata metadata;
    std::string adapter_id;
    std::string adapter_kind;   // "local", "webdav", "alist", "emby"
    std::string protocol;       // "file", "http", "https", "dav"
    std::string host;
    bool requires_auth{false};
    std::string credential_scope;
};
```

### SourceAdapterCapability

Declares what operations and features an adapter supports.

```cpp
struct SourceAdapterCapability {
    ContractMetadata metadata;
    std::string adapter_id;
    bool supports_range_read{false};
    bool supports_seek{false};
    bool supports_list_directory{false};
    bool supports_byte_range_cache{false};
    bool supports_connection_reuse{false};
    int64_t max_range_chunk_size{0};
};
```

### SourceAdapterFailure

Failure classification for source adapter operations.

```cpp
struct SourceAdapterFailure {
    ContractMetadata metadata;
    std::string adapter_id;
    std::string failure_kind;   // "connection_lost", "auth_failed", "timeout", "range_not_satisfiable", "redirect_loop", "tls_error", "not_found", "permission_denied"
    std::string failure_reason;
    bool is_recoverable{false};
    int retry_after_ms{0};
};
```

## Fake Service

Provides deterministic source adapter results for testing:

- `build_local_file_adapter()` - Local file system adapter
- `build_webdav_adapter()` - WebDAV adapter with range support
- `build_alist_adapter()` - AList adapter with direct link
- `build_emby_adapter()` - Emby adapter with session auth
- `build_failed_adapter()` - Adapter with connection failure

## Tests

1. Local file adapter identity
2. WebDAV adapter with range support
3. AList adapter with direct link
4. Emby adapter with session auth
5. Failed adapter with connection failure

## Evidence Target

source_adapter_core contracts.
