# P2-034 AList Provider Behavior Profile

## Purpose

Model the AList provider behavior profile, documenting how AList servers present their capabilities, supported operations, and failure modes. AList is a popular file listing/management tool that supports multiple storage backends.

## Scope

- AList server profile (version, capabilities, auth)
- AList API behavior (list, get, search, rename, move, copy, delete)
- AList failure modes (auth expired, rate limit, backend unavailable)
- AList probe result (server detection and profiling)

## Family Tree (source_core/)

```
source_core/
├── alist_server_profile.hpp       — AList server profile
├── alist_api_behavior.hpp         — AList API operation capabilities
├── alist_server_failure.hpp       — AList failure types
├── fake_alist_profile_service.hpp — Fake service for testing
```

## Contract Types

### AListServerProfile

| Field | Type | Description |
|-------|------|-------------|
| profile_id | string | Unique identifier |
| server_version | string | e.g. "alist/3.0" |
| site_name | string | Configured site name |
| enable_sign | bool | Whether signing is enabled |
| enable_web_dav | bool | Whether WebDAV is enabled |
| enable_sso | bool | Whether SSO is enabled |
| auth_method | string | "token", "password", "sso" |
| max_file_size | int64_t | Maximum uploadable file size |
| total_storage | int64_t | Total available storage |

### AListApiBehavior

| Field | Type | Description |
|-------|------|-------------|
| behavior_id | string | Unique identifier |
| supports_list | bool | Can list directory contents |
| supports_get | bool | Can get file/directory info |
| supports_search | bool | Can search files |
| supports_rename | bool | Can rename files |
| supports_move | bool | Can move files |
| supports_copy | bool | Can copy files |
| supports_delete | bool | Can delete files |
| supports_mkdir | bool | Can create directories |
| supports_link_generation | bool | Can generate direct download links |

### AListFailureKind (enum)

- AuthExpired: Token/session expired
- RateLimited: Rate limit exceeded
- BackendUnavailable: Storage backend unreachable
- PermissionDenied: Insufficient permissions
- NotFound: Resource not found

### AListServerFailure

| Field | Type | Description |
|-------|------|-------------|
| failure_kind | AListFailureKind | Type of failure |
| failure_reason | string | Human-readable reason |
| retry_after_ms | int64_t | Suggested retry delay |
| is_recoverable | bool | Whether retry might succeed |

## Tests

12 tests in `backend/tests/source_core/alist_profile_test.cpp`
