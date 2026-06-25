# P2-032 WebDAV Server Behavior Profile

## Purpose

Profile WebDAV server capabilities for remote playback. Different WebDAV servers
(nginx, Apache, Rclone, Synology, etc.) have different behaviors for range
requests, authentication, and content serving. This profile captures those
differences to enable reliable remote range-based playback.

## Contract Types

### WebDavServerProfile

Overall server profile discovered via PROPFIND/HEAD/OPTIONS probing.

| Field | Type | Description |
|-------|------|-------------|
| metadata | ContractMetadata | schema version |
| profile_id | string | unique profile identifier |
| server_version | string | e.g. "nginx/1.24", "Apache/2.4.58" |
| dav_compliance | string | DAV compliance class (1, 2) |
| supports_range_read | bool | server accepts Range header |
| supports_head_probe | bool | HEAD method works for probing |
| supports_propfind | bool | PROPFIND method available |
| supports_lock_unlock | bool | LOCK/UNLOCK available |
| auth_method | string | "none", "basic", "digest", "bearer" |
| max_content_length | int64_t | largest single GET allowed |

### WebDavRangeBehavior

Detailed range request behavior.

| Field | Type | Description |
|-------|------|-------------|
| metadata | ContractMetadata | schema version |
| range_header_supported | bool | Accept-Ranges: bytes present |
| content_range_present | bool | Content-Range header in 206 response |
| single_byte_range_only | bool | only one Range per request |
| chunk_size_limit | int64_t | max bytes per range request |
| status_on_full_content | int | status when Range not used |
| status_on_partial | int | status for satisfiable range |
| status_on_not_satisfiable | int | status for unsatisfiable range (416 or 200) |

### WebDavServerFailure

Failure modes specific to WebDAV probing.

| Field | Type | Description |
|-------|------|-------------|
| metadata | ContractMetadata | schema version |
| failure_kind | WebDavFailureKind | enum |
| failure_reason | string | human-readable |
| is_recoverable | bool | |

Enum `WebDavFailureKind`:
- ConnectionRefused
- AuthenticationRequired
- MethodNotAllowed
- PropfindUnsupported
- RangeNotSupported
- ContentLengthUnknown
- ServerTimeout
- SslCertificateError

### WebDavProbeResult

Outcome of profiling a WebDAV endpoint.

| Field | Type | Description |
|-------|------|-------------|
| metadata | ContractMetadata | schema version |
| profile_id | string | |
| server_profile | WebDavServerProfile | |
| range_behavior | WebDavRangeBehavior | |
| probe_latency_ms | int64_t | |
| probe_success | bool | |
| failure | optional<WebDavServerFailure> | |

## Fake Service Scenarios

| Builder | Scenario |
|---------|----------|
| build_nginx_profile | nginx with range support |
| build_apache_profile | Apache with range support |
| build_rclone_profile | Rclone WebDAV mode |
| build_synology_profile | Synology NAS DAV |
| build_no_range_profile | server without range |
| build_nginx_range_behavior | full range behavior |
| build_partial_range_behavior | limited chunk size |
| build_no_range_behavior | range not supported |
| build_connection_failure | connection refused |
| build_auth_failure | authentication required |
| build_range_not_supported_failure | method not allowed |
| build_successful_probe | full probe success |
| build_failed_probe | probe with failure |

## Test Count

15 tests covering profiles, range behaviors, failures, and probe results.
