# KIVO-VIDEO-P2-SOURCE-CORE-PLANNING-001

## Task ID
KIVO-VIDEO-P2-SOURCE-CORE-PLANNING-001

## Status
CONTRACT_PASS — planning-only, no implementation

## Classification
PASS_P2_SOURCE_CORE_PLANNING_001

---

## 1. Scope Gate

### 1.1 What Source Core Is

P2 Source Core is the **unified abstraction layer** that models every playback source
(local file, network protocol, cloud drive, media server, live stream) as a single
family of contracts. It answers: _what is a source, how do we open it, read from it,
seek in it, authenticate to it, and produce evidence about it?_

Source Core does NOT implement providers. It defines the contract vocabulary that
every provider must satisfy.

### 1.2 In-Scope

| # | Module | Responsibility |
|---|---|---|
| 1 | Source Identity | source_id, canonical URI, display URI, fingerprint, provenance, redaction |
| 2 | Provider Kind Model | 21 provider kinds with typed enum + capability hints per kind |
| 3 | Source URI | canonicalization, redaction, validation, display forms |
| 4 | Source Capability Probe | auth required/available, seekable, range support, content length, latency class |
| 5 | Source Open Contract | open request → result, failure taxonomy, retry, timeout, cancellation |
| 6 | Source Read Contract | read_at, sequential read, range read, EOF, backpressure |
| 7 | Source Seek Contract | absolute seek, relative seek, seekable check, position tracking |
| 8 | Source Cache/Buffer Boundary | cache hint only — no buffer implementation |
| 9 | Source Auth Contract | auth state, credential reference, redaction policy, auth failure |
| 10 | Source Redaction | URI redaction, credential redaction, path redaction, evidence redaction |
| 11 | Source Evidence | machine-readable snapshots for Inspector/DecisionLedger |
| 12 | Direct Play / Direct Stream Input | typed evidence inputs for decision engine — no decision logic |
| 13 | Source Error Taxonomy | failure kinds, error codes, retryable/non-retryable, recovery hints |

### 1.3 Out-of-Scope (Non-goals)

- ❌ Emby API implementation
- ❌ Alist API implementation
- ❌ WebDAV client implementation
- ❌ SMB/NFS client implementation
- ❌ HTTP downloader implementation
- ❌ FFmpeg demux/decode integration
- ❌ D3D11 video render
- ❌ WASAPI audio output
- ❌ Presenter/renderer
- ❌ Qt UI
- ❌ Playback runtime
- ❌ E2E / Soak testing
- ❌ Golden sample management
- ❌ Provider runtime verification
- ❌ Decision engine (DirectPlayDecision, DirectStreamDecision)
- ❌ Any API server implementation

---

## 2. Inventory Report

### 2.1 Existing Source-Related Files

The current `backend/include/kivo/cinema_engine/source_core/` has **41 files**
in a FLAT structure with no subdirectories. This is the primary concern — the
directory is already oversized and unstructured.

| File | Issue |
|---|---|
| `source_adapter_1.hpp` – `source_adapter_4.hpp` | Adapter contract files — valid |
| `local_file_identity.hpp` | Provider identity mixed with core contracts |
| `emby_direct_play_*.hpp` (3 files) | Emby-specific in generic source_core/ |
| `emby_session_*.hpp` (2 files) | Emby-specific in generic source_core/ |
| `alist_direct_link_*.hpp` (5 files) | Alist-specific in generic source_core/ |
| `alist_server_*.hpp` (2 files) | Alist-specific in generic source_core/ |
| `webdav_range_*.hpp` (5 files) | WebDAV-specific in generic source_core/ |
| `webdav_server_profile.hpp` / `webdav_probe_result.hpp` | WebDAV-specific |
| `network_cache_*.hpp` (3 files) | Mixed network concern |
| `network_range_reader_*.hpp` (3 files) | Mixed network concern |
| `source_health_*.hpp` (4 files) | Health metrics — valid but flat |
| `credential_policy.hpp`, `tls_policy.hpp`, `ssrf_policy.hpp` | Security policies in source_core/ |

### 2.2 Structural Problems Identified

| Problem | Severity | Description |
|---|---|---|
| **Flat directory** | CRITICAL | 41 files in one directory, no subdirectory structure |
| **Provider-specific contracts in core** | CRITICAL | Emby/AList/WebDAV type definitions co-located with core contracts |
| **No family separation** | HIGH | Identity, capability, open, read, seek all mixed together |
| **Mixed abstraction levels** | HIGH | Core contracts and provider-specific types share the same namespace |
| **No redaction contract** | MEDIUM | No dedicated redaction types despite multiple files needing them |
| **No URI contract** | MEDIUM | No source_uri, canonical_uri, or redacted_uri types |
| **No evidence contract** | MEDIUM | No typed evidence snapshots for decision engine input |

---

## 3. Family Tree Planning

### 3.1 Directory Structure

```
backend/include/kivo/video/source_core/
├── identity/
│   ├── source_identity.hpp          # SourceIdentity struct
│   ├── source_provenance.hpp        # SourceProvenance enum
│   └── source_fingerprint.hpp       # SourceFingerprint (content hash + metadata)
├── provider_kind/
│   ├── provider_kind.hpp            # ProviderKind enum (21 values)
│   └── provider_kind_traits.hpp     # Capability hints per provider kind
├── uri/
│   ├── source_uri.hpp               # SourceUri (raw input URI)
│   ├── canonical_source_uri.hpp     # CanonicalSourceUri (normalized)
│   └── redacted_source_uri.hpp      # RedactedSourceUri (safe for evidence)
├── capability/
│   ├── source_capability_snapshot.hpp  # SourceCapabilitySnapshot
│   └── capability_probe_result.hpp     # CapabilityProbeResult
├── open/
│   ├── source_open_request.hpp      # SourceOpenRequest
│   ├── source_open_result.hpp       # SourceOpenResult
│   └── source_open_failure.hpp      # SourceOpenFailure (failure taxonomy)
├── read/
│   ├── source_read_request.hpp      # SourceReadRequest (offset + length)
│   ├── source_read_result.hpp       # SourceReadResult (data + bytes_read)
│   └── source_read_failure.hpp      # SourceReadFailure
├── seek/
│   ├── source_seek_request.hpp      # SourceSeekRequest
│   ├── source_seek_result.hpp       # SourceSeekResult
│   └── source_seek_failure.hpp      # SourceSeekFailure
├── cache/
│   └── source_cache_hint.hpp        # SourceCacheHint (boundary only)
├── auth/
│   ├── source_auth_state.hpp        # SourceAuthState enum + struct
│   ├── source_credential_ref.hpp    # SourceCredentialRef (redacted reference)
│   └── source_auth_failure.hpp      # SourceAuthFailure
├── redaction/
│   ├── source_redaction_policy.hpp  # SourceRedactionPolicy
│   └── redaction_level.hpp          # RedactionLevel enum
├── error/
│   ├── source_error.hpp             # SourceError (error code + message)
│   └── source_failure_kind.hpp      # SourceFailureKind enum
├── evidence/
│   ├── source_evidence_snapshot.hpp # SourceEvidenceSnapshot
│   └── source_evidence_error.hpp    # SourceEvidenceError
├── decision_input/
│   ├── direct_play_source_input.hpp # DirectPlaySourceInput
│   └── direct_stream_source_input.hpp # DirectStreamSourceInput

backend/src/source_core/
├── identity/          (mirror header structure)
├── provider_kind/
├── uri/
├── capability/
├── open/
├── read/
├── seek/
├── cache/
├── auth/
├── redaction/
├── error/
├── evidence/
└── decision_input/

backend/tests/source_core/
├── identity/
│   └── source_identity_contract_tests.cpp
├── provider_kind/
│   └── provider_kind_contract_tests.cpp
├── uri/
│   └── source_uri_redaction_tests.cpp
├── capability/
│   └── capability_snapshot_tests.cpp
├── open/
│   └── source_open_contract_tests.cpp
├── read/
│   └── source_read_contract_tests.cpp
├── seek/
│   └── source_seek_contract_tests.cpp
├── auth/
│   └── source_auth_tests.cpp
├── redaction/
│   └── source_redaction_tests.cpp
├── error/
│   └── source_error_taxonomy_tests.cpp
├── evidence/
│   └── source_evidence_schema_tests.cpp
└── decision_input/
    └── source_decision_input_tests.cpp
```

### 3.2 Family Tree Principles

| Rule | Description |
|---|---|
| **No flat directories** | Every family is a subdirectory with ≤5 files |
| **No "common/" "utils/" "helpers/"** | Every file has a single, named responsibility |
| **No "manager" files** | No source_manager.cpp, provider_manager.cpp |
| **Provider-specific code is SEPARATE** | Providers live in their own families, not inside source_core/ |
| **Contract-first** | Every family starts with its contract header, then implementation |
| **Test mirrors production** | Test directory tree mirrors production tree 1:1 |

---

## 4. Single-File Responsibility Table

| # | File Path | Family | Single Responsibility | Input | Output | Forbidden Content | Cross-Family Deps | Max Lines |
|---|---|---|---|---|---|---|---|---|
| 1 | `identity/source_identity.hpp` | identity | SourceIdentity: id + kind + fingerprint ref | — | SourceIdentity struct | provider runtime, paths, tokens | provider_kind | 80 |
| 2 | `identity/source_provenance.hpp` | identity | SourceProvenance: 5-value enum (local/remote/cache/generated/unknown) | — | SourceProvenance enum | production timestamps | none | 40 |
| 3 | `identity/source_fingerprint.hpp` | identity | SourceFingerprint: content hash + size + mtime | — | SourceFingerprint struct | raw paths | none | 60 |
| 4 | `provider_kind/provider_kind.hpp` | provider_kind | ProviderKind: 21-value enum | — | ProviderKind enum | runtime detection logic | none | 60 |
| 5 | `provider_kind/provider_kind_traits.hpp` | provider_kind | Static capability hints per ProviderKind | ProviderKind | seekable hint, range hint, auth hint | runtime probe results | provider_kind | 100 |
| 6 | `uri/source_uri.hpp` | uri | SourceUri: raw input URI with validation | string | SourceUri struct | network resolution | provider_kind | 90 |
| 7 | `uri/canonical_source_uri.hpp` | uri | CanonicalSourceUri: normalized URI | SourceUri | CanonicalSourceUri | redaction | uri, redaction | 70 |
| 8 | `uri/redacted_source_uri.hpp` | uri | RedactedSourceUri: safe-for-evidence URI | CanonicalSourceUri | RedactedSourceUri | raw paths, tokens | uri, redaction | 80 |
| 9 | `capability/source_capability_snapshot.hpp` | capability | SourceCapabilitySnapshot: 15 capability flags | — | SourceCapabilitySnapshot struct | runtime probe implementation | identity, provider_kind | 120 |
| 10 | `capability/capability_probe_result.hpp` | capability | CapabilityProbeResult: probe outcome + confidence | — | CapabilityProbeResult | provider-specific logic | capability | 90 |
| 11 | `open/source_open_request.hpp` | open | SourceOpenRequest: URI + auth ref + timeout + policy | SourceUri, SourceAuthState | SourceOpenRequest | credentials, tokens | uri, auth | 100 |
| 12 | `open/source_open_result.hpp` | open | SourceOpenResult: handle + capacity + capability | — | SourceOpenResult | platform handles | capability | 90 |
| 13 | `open/source_open_failure.hpp` | open | SourceOpenFailure: failure kind + retry + message | — | SourceOpenFailure | provider-specific error codes | error | 80 |
| 14 | `read/source_read_request.hpp` | read | SourceReadRequest: offset + length + buffer hint | — | SourceReadRequest | actual buffers | none | 60 |
| 15 | `read/source_read_result.hpp` | read | SourceReadResult: bytes_read + EOF flag | — | SourceReadResult | raw data buffers | none | 50 |
| 16 | `read/source_read_failure.hpp` | read | SourceReadFailure: failure kind + partial read | — | SourceReadFailure | error recovery logic | error | 60 |
| 17 | `seek/source_seek_request.hpp` | seek | SourceSeekRequest: offset + whence | — | SourceSeekRequest | actual seek implementation | none | 50 |
| 18 | `seek/source_seek_result.hpp` | seek | SourceSeekResult: new_position + success | — | SourceSeekResult | position tracking | none | 50 |
| 19 | `seek/source_seek_failure.hpp` | seek | SourceSeekFailure: failure kind + position | — | SourceSeekFailure | retry logic | error | 60 |
| 20 | `cache/source_cache_hint.hpp` | cache | SourceCacheHint: cache_policy + TTL + eviction hint | — | SourceCacheHint | cache implementation | capability | 80 |
| 21 | `auth/source_auth_state.hpp` | auth | SourceAuthState: auth required + auth available + state | — | SourceAuthState enum+struct | actual credentials | provider_kind | 70 |
| 22 | `auth/source_credential_ref.hpp` | auth | SourceCredentialRef: redacted credential identifier | — | SourceCredentialRef | raw tokens, passwords | redaction | 60 |
| 23 | `auth/source_auth_failure.hpp` | auth | SourceAuthFailure: failure kind + redacted message | — | SourceAuthFailure | raw auth headers | error, redaction | 60 |
| 24 | `redaction/source_redaction_policy.hpp` | redaction | SourceRedactionPolicy: what to redact + level | provider_kind | SourceRedactionPolicy | default-allow | provider_kind | 80 |
| 25 | `redaction/redaction_level.hpp` | redaction | RedactionLevel: none/partial/full | — | RedactionLevel enum | per-field granularity | none | 30 |
| 26 | `error/source_error.hpp` | error | SourceError: error_code + domain + message | — | SourceError struct | recovery logic | none | 60 |
| 27 | `error/source_failure_kind.hpp` | error | SourceFailureKind: 15-value enum (auth/network/not_found/timeout/...) | — | SourceFailureKind enum | implementation errors | none | 60 |
| 28 | `evidence/source_evidence_snapshot.hpp` | evidence | SourceEvidenceSnapshot: all redacted evidence fields | contract outputs | SourceEvidenceSnapshot | raw credentials, raw paths | redaction, identity, capability | 150 |
| 29 | `evidence/source_evidence_error.hpp` | evidence | SourceEvidenceError: evidence generation failure | — | SourceEvidenceError | suppressed errors | error | 50 |
| 30 | `decision_input/direct_play_source_input.hpp` | decision_input | DirectPlaySourceInput: container match + codec match + seekable | SourceEvidenceSnapshot | DirectPlaySourceInput | decision logic, player integration | evidence | 100 |
| 31 | `decision_input/direct_stream_source_input.hpp` | decision_input | DirectStreamSourceInput: transcode hint + bandwidth + format | SourceEvidenceSnapshot | DirectStreamSourceInput | decision logic, transcoding selection | evidence | 100 |

**Total: 31 planned files across 13 families. Average ~76 lines per file.**

---

## 5. Dependency Direction Plan

```
                    provider_kind  (leaf — 0 deps)
                         ↑
                    ┌────┴────┐
                    ↓         ↓
               redaction    uri
                    ↓         ↓
                    └────┬────┘
                         ↓
                     identity
                         ↓
                  ┌──────┴──────┐
                  ↓             ↓
              capability       auth
                  ↓             ↓
            ┌─────┴─────┐     ┌──┴──┐
            ↓           ↓     ↓     ↓
          open         cache  error  (shared)
            ↓
       ┌────┴────┐
       ↓         ↓
     read       seek
       ↓         ↓
       └────┬────┘
            ↓
        evidence
            ↓
     decision_input
```

### Allowed Dependencies

| From | May Depend On |
|---|---|
| provider_kind | (none) |
| redaction | provider_kind |
| uri | provider_kind, redaction |
| identity | provider_kind, uri, redaction |
| capability | identity, provider_kind |
| auth | provider_kind, redaction |
| open | identity, capability, auth, error |
| cache | identity, capability |
| read | open, error |
| seek | read, capability, error |
| error | (none — shared vocabulary) |
| evidence | identity, capability, open, read, seek, error, redaction |
| decision_input | identity, capability, evidence |

### Forbidden Reverse Dependencies

| From | Must NOT Depend On |
|---|---|
| identity | open, read, seek, evidence, decision_input |
| provider_kind | any other family |
| redaction | provider runtime, evidence |
| uri | open, read, seek |
| capability | open, read, seek, auth |
| evidence | actual providers, API clients |
| decision_input | FFmpeg, D3D11, WASAPI, presenter, UI |

---

## 6. Contract Planning

### 6.1 SourceIdentity

```cpp
struct SourceIdentity {
    SourceId id;                      // uint64_t, monotonic
    ProviderKind provider_kind;       // enum
    CanonicalSourceUri canonical_uri;
    RedactedSourceUri display_uri;
    SourceFingerprint fingerprint;    // nullable
    SourceProvenance provenance;      // local/remote/cache/generated
    bool content_verified;            // fingerprint validated
};
```

- **Forbidden states**: id=0, empty URI, provider_kind=UNKNOWN without explicit marker
- **Redline**: display_uri must never contain raw path or token
- **Evidence**: id + provider_kind + display_uri (redacted — no Windows/Unix user paths)

### 6.2 ProviderKind

```cpp
enum class ProviderKind : uint8_t {
    LOCAL_FILE, SMB, NFS,
    FTP, FTPS, SFTP,
    HTTP_FILE, HTTPS_FILE, HTTP_STREAM, HTTPS_STREAM,
    WEBDAV, ALIST, EMBY, PLEX, JELLYFIN,
    GOOGLE_DRIVE, ONEDRIVE,
    S3_OBJECT, MINIO_OBJECT,
    IPTV_M3U, HLS_LIVE,
    DLNA_UPNP,
    UNKNOWN = 0xFF
};
```

- **Forbidden states**: UNKNOWN used as default without explicit marker
- **Redline**: Must be a typed enum, not a string
- **Expansion**: New providers add enum values; never change existing ordinals

### 6.3 SourceUri Family

| Contract | Purpose | Fields |
|---|---|---|
| SourceUri | Raw input URI | raw_string, provider_kind, is_valid |
| CanonicalSourceUri | Normalized | normalized_string, scheme, host, port, path |
| RedactedSourceUri | Safe for evidence | display_string (no tokens, no user paths, no secrets) |

- **Forbidden**: raw_string in evidence; <REDACTED_WIN_PATH> or <REDACTED_UNIX_PATH>/ in any URI form
- **Redline**: CanonicalSourceUri must never contain query params with tokens

### 6.4 SourceCapabilitySnapshot

| Field | Type | Meaning |
|---|---|---|
| is_seekable | bool | Source supports seeking |
| supports_range_read | bool | HTTP Range / protocol range |
| content_length_known | bool | Content-Length available |
| content_length_bytes | uint64_t | Known content length |
| auth_required | bool | Authentication needed |
| auth_available | bool | Credentials available |
| is_reconnectable | bool | Supports reconnect after disconnect |
| latency_class | LatencyClass | Local / LAN / WAN / Unknown |
| source_class | SourceClass | File / Stream / Live |
| mime_hint | string | Content-Type hint from probe |
| container_hint | string | Container format hint |
| has_subtitle_sidecar | bool | Sidecar subtitles detected |
| has_chapter_sidecar | bool | Sidecar chapters detected |
| direct_play_eligible | bool | Eligible for Direct Play (provisional) |
| direct_stream_eligible | bool | Eligible for Direct Stream (provisional) |

- **Forbidden states**: direct_play_eligible=true without source evidence
- **Redline**: mime_hint must not reveal user content names

### 6.5 Source Open Contracts

| Contract | Key Fields |
|---|---|
| SourceOpenRequest | source_uri, credential_ref, timeout_ms, retry_policy, cancel_token |
| SourceOpenResult | source_handle, capability_snapshot, auth_state |
| SourceOpenFailure | failure_kind, retryable, retry_after_ms, error_message |

- **Forbidden**: source_handle exposing platform handles (HANDLE, fd, socket)
- **Redline**: error_message redacted per redaction policy

### 6.6 Source Read Contracts

| Contract | Key Fields |
|---|---|
| SourceReadRequest | offset, length, buffer_size_hint |
| SourceReadResult | bytes_read, is_eof, data_length |
| SourceReadFailure | failure_kind, bytes_read_partial, retryable |

### 6.7 Source Seek Contracts

| Contract | Key Fields |
|---|---|
| SourceSeekRequest | target_offset, seek_mode (ABSOLUTE/RELATIVE/TO_END) |
| SourceSeekResult | new_position, success |
| SourceSeekFailure | failure_kind, attempted_position |

### 6.8 Source Auth Contracts

| Contract | Key Fields |
|---|---|
| SourceAuthState | provider_kind, auth_required, auth_available, auth_method, state |
| SourceCredentialRef | credential_id (redacted), credential_type, scope_hint |
| SourceAuthFailure | failure_kind, is_token_expired, is_refreshable |

- **Forbidden**: raw token, raw password, raw API key in any field
- **Redline**: credential_id must be a redacted reference, not the actual credential

### 6.9 Source Cache Contract

| Contract | Key Fields |
|---|---|
| SourceCacheHint | cache_policy (NO_CACHE / MEMORY / DISK), ttl_seconds, max_size_bytes, eviction_policy |

- **Forbidden**: cache implementation, disk I/O, buffer management
- **Redline**: cache_policy is a hint, not a command — decision engine decides

### 6.10 Source Error Contracts

| Contract | Key Fields |
|---|---|
| SourceError | error_code, domain, message (redacted), timestamp |
| SourceFailureKind | 15 enum values: AUTH_FAILED, NETWORK_UNREACHABLE, TIMEOUT, NOT_FOUND, PERMISSION_DENIED, CONNECTION_REFUSED, TLS_ERROR, REDIRECT_LOOP, CONTENT_TOO_LARGE, RANGE_NOT_SUPPORTED, PROTOCOL_ERROR, INTERNAL_ERROR, CANCELLED, UNSUPPORTED_PROVIDER, UNKNOWN |

### 6.11 Source Evidence Contracts

| Contract | Key Fields |
|---|---|
| SourceEvidenceSnapshot | source_id, provider_kind, uri_redacted, capability, auth_state_redacted, open_result, read_stats, seekability, range_support, error_history, direct_play_input, direct_stream_input, runtime_verified, runtime_environment_kind, redaction_verified, schema_validated, timestamp |
| SourceEvidenceError | error_code, message |

- **Forbidden**: runtime_verified=true without provider runtime execution
- **Redline**: All URI/path/token/auth fields must be redacted

### 6.12 Decision Input Contracts

| Contract | Key Fields |
|---|---|
| DirectPlaySourceInput | source_id, container_format, codec_family, is_seekable, content_length, bandwidth_estimate, mime_type |
| DirectStreamSourceInput | source_id, transcode_required_hint, target_format_hint, bandwidth_available, max_resolution_hint, max_bitrate_hint |

- **Forbidden**: Decision logic — Source Core only produces INPUTS
- **Redline**: direct_play_eligible / direct_stream_eligible are provisional hints, not decisions

---

## 7. Provider Family Boundary

Each provider family is a SEPARATE directory, NOT inside source_core/. Source Core
defines contracts; providers implement them.

### 7.1 Local File Provider

**Directory**: `backend/include/kivo/video/providers/local_file/`

| Contract | Purpose |
|---|---|
| LocalFileIdentity | Path canonicalization + existence + size + mtime |
| LocalFileProbe | Fast file stat probe |
| LocalFileRedaction | Redact local paths for evidence |

- **Forbidden**: raw <REDACTED_WIN_PATH> or <REDACTED_UNIX_PATH>/ in evidence
- **Status in this planning**: Boundary defined, not implemented

### 7.2 HTTP Provider

**Directory**: `backend/include/kivo/video/providers/http/`

| Contract | Purpose |
|---|---|
| HttpSourceUri | URL canonicalization + redirect handling |
| HttpHeadProbe | HEAD request → content-length, accept-ranges, content-type |
| HttpRangeSupport | Range header negotiation |
| HttpAuthRedaction | Redact Authorization headers |

### 7.3 WebDAV Provider

**Directory**: `backend/include/kivo/video/providers/webdav/`

| Contract | Purpose |
|---|---|
| WebDavProbe | PROPFIND → resource type + size + etag |
| WebDavRangeRead | Range-based partial read |
| WebDavCredential | Redacted credential reference |

### 7.4 Alist Provider

**Directory**: `backend/include/kivo/video/providers/alist/`

| Contract | Purpose |
|---|---|
| AlistAuth | Token-based auth boundary |
| AlistDirectLink | Direct download URL extraction |
| AlistCachePolicy | Cache hint for Alist content |

- **Forbidden**: Alist API implementation; raw fs_id/sign in evidence

### 7.5 Emby / Jellyfin / Plex Provider

**Directory**: `backend/include/kivo/video/providers/emby/` (Emby as reference)

| Contract | Purpose |
|---|---|
| EmbyMediaIdentity | ItemId + server_id + media_source_id |
| EmbyPlaybackInfo | Playback info boundary — is_direct_play, transcode hints |
| EmbyAuthRedaction | Redact server URL + token |

- **Forbidden**: Emby API client implementation

### 7.6 Cloud Drive Provider

**Directory**: `backend/include/kivo/video/providers/cloud_drive/`

| Contract | Purpose |
|---|---|
| CloudDriveToken | Token reference (redacted) — no raw token |
| CloudDriveMetadata | File metadata probe |
| CloudDriveDownloadUrl | Download URL boundary |

### 7.7 IPTV / HLS Provider

**Directory**: `backend/include/kivo/video/providers/iptv_hls/`

| Contract | Purpose |
|---|---|
| PlaylistIdentity | M3U playlist identity + source |
| LiveSeekability | Live stream seekability assessment |
| SegmentSourceHint | Segment-based source boundary |

---

## 8. Evidence Planning

### 8.1 Evidence JSON Output

```json
{
  "schema_version": "p2-evidence-v8",
  "task_id": "SRC-CORE-PLANNING-001",
  "task_family": "source_core",
  "status": "CONTRACT_PASS",
  "completion_level": "p2-foundation-planning",
  "runtime_environment_kind": "not_applicable",
  "runtime_verified": false,
  "environment": {},
  "implementation": {
    "source_identity": "planning-only",
    "family_tree": "13 subdirectories, 31 planned files",
    "contracts_planned": 25,
    "providers_boundary_defined": 7
  },
  "evidence_snapshot": {
    "source_identity_defined": true,
    "provider_kinds": 21,
    "contracts": ["SourceIdentity", "ProviderKind", "SourceUri", "CanonicalSourceUri", "RedactedSourceUri", "SourceCapabilitySnapshot", "SourceOpenRequest", "SourceOpenResult", "SourceOpenFailure", "SourceReadRequest", "SourceReadResult", "SourceReadFailure", "SourceSeekRequest", "SourceSeekResult", "SourceSeekFailure", "SourceAuthState", "SourceCredentialRef", "SourceAuthFailure", "SourceError", "SourceFailureKind", "SourceCacheHint", "DirectPlaySourceInput", "DirectStreamSourceInput", "SourceEvidenceSnapshot", "SourceRedactionPolicy"],
    "provider_families_boundary": ["local_file", "http", "webdav", "alist", "emby", "cloud_drive", "iptv_hls"],
    "dependency_direction_validated": true,
    "no_flat_directories": true,
    "no_common_utils_helper_files": true
  },
  "redaction_verified": true,
  "schema_validated": true
}
```

### 8.2 Evidence Rules

| Rule | Description |
|---|---|
| CONTRACT_PASS only | Planning evidence uses CONTRACT_PASS, never RUNTIME_PASS |
| runtime_verified = false | No provider runtime execution in planning phase |
| not_applicable env | No runtime environment needed for planning |
| Redacted content | No paths, tokens, secrets in evidence |
| Machine-readable | Valid JSON against p2-evidence-v8 schema |

---

## 9. Test Planning

| Test Family | Positive Cases | Negative Cases | Redline Cases | Forbidden Token Cases |
|---|---|---|---|---|
| source_identity_contract | valid id + canonical URI + fingerprint | empty id, unknown provider, raw path in display_uri | <REDACTED_WIN_PATH> in any field, token in URI | <REDACTED_WIN_PATH>, <REDACTED_UNIX_PATH>/, <REDACTED_HOME>/, Bearer, <REDACTED_KEY>= |
| provider_kind_contract | all 21 enum values constructible | string-to-enum, invalid ordinals | UNKNOWN as default without marker | stringly-typed kind |
| source_uri_redaction | redacted URI preserves scheme+host, removes query+token | raw path leaked, token preserved | <REDACTED_WIN_PATH> in redacted output | <REDACTED_TOKEN>, api_key, secret |
| capability_snapshot | all 15 fields with valid defaults | direct_play=true without evidence | BLOCKED_ENV claimed as PASS | SKIPPED_TEST_ONLY as PASS |
| source_open_contract | valid request → result with handle | timeout → failure, auth_failed → retryable | silent fallback to RUNTIME_PASS | mock runtime claimed as real |
| source_read_contract | sequential read → bytes_read, eof → true | read past EOF → failure, offset OOB | backpressure ignored | silent truncation |
| source_seek_contract | absolute seek → success, relative seek → success | seek past EOF → failure, unsupported → failure | seek ignored without error | TO_END on non-seekable |
| source_auth_contract | auth state transitions, credential ref | expired token → auth_failed, refresh → new state | raw token in credential_ref | password, secret visible |
| source_error_taxonomy | all 15 failure kinds constructible | unknown failure kind | BLOCKED_ENV as error | RUNTIME_PASS with error |
| source_evidence_schema | valid JSON, all required fields, redacted | schema violation, missing fields | runtime_verified=true without proof | token/secret/path in evidence |
| source_decision_input | direct_play_input with valid evidence, direct_stream_input | direct_play=true without seekable, content_length unknown but claimed known | mock evidence produces RUNTIME_PASS | null evidence → direct_play=true |

---

## 10. Redline Rules

### Source Core Redlines

| # | Rule | Violation |
|---|---|---|
| R1 | No RUNTIME_PASS without real provider runtime execution | fake pass |
| R2 | No direct_play=true without source evidence proving eligibility | premature claim |
| R3 | No direct_stream=true without source evidence proving eligibility | premature claim |
| R4 | No raw credential in any evidence field | credential leak |
| R5 | No raw user path (<REDACTED_WIN_PATH>, <REDACTED_UNIX_PATH>/, <REDACTED_HOME>/) in any evidence field | path leak |
| R6 | No silent fallback — capability_unknown → probe, not assume | silent degradation |
| R7 | No unsupported provider marked as supported | false capability |
| R8 | No BLOCKED_ENV treated as PASS | blocked as pass |
| R9 | No SKIPPED_TEST_ONLY treated as PASS | skip as pass |
| R10 | No mock/simulator claimed as real runtime | mock runtime |
| R11 | No HTTP 200 assumed seekable without Range header verification | false seekable |
| R12 | No provider integration claimed before implementation exists | premature integration |
| R13 | No cache hit treated as provider runtime proof | cache as proof |
| R14 | No stringly-typed provider_kind — must be typed enum | string over enum |

---

## 11. Validation

Run before commit:

```powershell
python backend/tools/governance/redline_gate/redline_checker.py --self-test
python backend/tools/governance/redline_gate/redline_checker.py --root=. --scan=all
```

Requirements:
- self-test: 16/16 PASS
- scan all: PASS, violations=0
- Working tree: only allowed files (planning doc, memory, checklist)

No build required. No CTest required. No E2E/Soak required.

---

## 12. Final Statement

**This document is P2 Source Core PLANNING ONLY.**

This does NOT implement Source Core.
This does NOT mean P2 Source Core is complete.
This does NOT mean All Providers Runtime Verified.
This does NOT mean P2 readiness.
This does NOT mean P3/HDR/Atmos/AirPlay complete.

Next task: KIVO-VIDEO-P2-SOURCE-CORE-BOOTSTRAP-001 (contract header creation).
