# P2 Provider Runtime Matrix V8 Schema

Updated: 2026-06-26

This document defines the `provider-runtime-matrix-v8` schema for tracking provider runtime verification status.

## Schema Name

`provider-runtime-matrix-v8`

## Purpose

The provider runtime matrix tracks the runtime verification status of all P2 providers, ensuring honest representation of what has been verified in real environments versus what remains stub/contract-only.

## Top-Level Fields

1. `schema_version` (string, required): Always "provider-runtime-matrix-v8".
2. `generated_timestamp` (string, required): ISO 8601 timestamp of matrix generation.
3. `generation_commit` (string, required): Git commit hash used for generation.
4. `total_providers` (integer, required): Total number of providers in matrix.
5. `verified_providers` (integer, required): Number of providers with RUNTIME_PASS status.
6. `blocked_providers` (integer, required): Number of providers with BLOCKED_ENV status.
7. `providers` (array, required): Array of provider entries.

## Provider Entry Fields

Each provider entry contains:

1. `provider_kind` (string, required): Provider identifier from vocabulary.
2. `provider_name` (string, required): Human-readable provider name.
3. `contract_status` (string, required): Contract implementation status.
4. `runtime_status` (string, required): Runtime verification status from V8 vocabulary.
5. `environment_kind` (string, required): Environment kind from V8 vocabulary.
6. `environment_required` (boolean, required): Whether real environment is required.
7. `environment_available` (boolean, required): Whether real environment is available.
8. `blocked_reason` (string, nullable): Reason if blocked.
9. `runtime_verified_timestamp` (string, nullable): When runtime verification occurred.
10. `evidence_file` (string, nullable): Path to evidence JSON file.
11. `provider_matrix_impact` (object, optional): Impact on other providers.
12. `commands` (array, optional): Supported commands/operations.
13. `limitations` (array, optional): Known limitations.

## Provider Kind Vocabulary

### Local Providers
1. `local_file`: Local file playback (MP4, MKV, etc.)
2. `disc_image`: Disc image playback (BDMV, ISO)

### Network File Providers
3. `smb`: SMB/CIFS network file shares
4. `nfs`: NFS network file shares
5. `ftp`: FTP file transfer
6. `ftps`: FTP over TLS
7. `sftp`: SSH File Transfer Protocol
8. `http_file`: HTTP/HTTPS direct file
9. `https_file`: HTTPS direct file

### Object Storage Providers
10. `s3_object`: Amazon S3-compatible object storage
11. `minio_object`: MinIO object storage

### WebDAV/AList Providers
12. `webdav`: WebDAV server
13. `alist`: AList file listing server

### Media Server Providers
14. `emby`: Emby media server
15. `plex`: Plex media server
16. `jellyfin`: Jellyfin media server

### Cloud Drive Providers
17. `google_drive`: Google Drive
18. `onedrive`: Microsoft OneDrive

### Live Streaming Providers
19. `iptv_hls`: IPTV HLS live streams

### Presentation Extension Providers
20. `airplay_cast`: AirPlay/Google Cast
21. `trakt_api`: Trakt API integration

## Contract Status Vocabulary

1. `CONTRACT_PASS`: Contract headers and stubs implemented.
2. `NOT_IMPLEMENTED`: No contract headers exist.
3. `FAIL`: Contract implementation has errors.

## Environment Kind Vocabulary

1. `test_harness`: Controlled test environment.
2. `local_machine`: Real local machine with hardware.
3. `remote_server`: Real remote server.
4. `cloud_account`: Real cloud account with credentials.
5. `simulated`: Simulated environment (not real).
6. `none`: No environment required.

## Runtime Status Rules

1. `RUNTIME_PASS`: Only allowed with real environment verification.
2. `CONTRACT_PASS`: Allowed for stub/contract implementations.
3. `BLOCKED_ENV`: Environment not available for verification.
4. `FAIL`: Verification failed.
5. `NOT_IMPLEMENTED`: No implementation exists.
6. `SKIPPED_TEST_ONLY`: Test skipped, not runtime proof.

## Anti-Fake Rules

1. No provider defaults to `RUNTIME_PASS` without evidence.
2. Missing real external environment must be `BLOCKED_ENV`.
3. Controlled protocol server cannot be mislabeled as cloud account.
4. Stub/simulator cannot be `RUNTIME_PASS`.
5. Test harness cannot be `RUNTIME_PASS` for providers requiring real environment.
6. No provider can claim `RUNTIME_PASS` without `evidence_file`.

## Provider Matrix Generation Rules

1. Matrix must be generated from source code analysis.
2. Provider kinds must match task family mapping.
3. Evidence files must exist for `RUNTIME_PASS` providers.
4. Blocked reasons must be specific and honest.
5. Timestamps must be ISO 8601 format.
6. Commit hash must be valid Git SHA.

## Validation Rules

1. All required top-level fields must be present.
2. `provider_kind` must be from provider vocabulary.
3. `contract_status` must be from contract status vocabulary.
4. `runtime_status` must be from V8 status vocabulary.
5. `environment_kind` must be from environment kind vocabulary.
6. `RUNTIME_PASS` requires `evidence_file` and `runtime_verified_timestamp`.
7. `BLOCKED_ENV` requires `blocked_reason`.
8. Total counts must match array length.

## Negative Test Cases

1. Provider with `RUNTIME_PASS` but no `evidence_file` → FAIL.
2. Provider with `RUNTIME_PASS` but `environment_available: false` → FAIL.
3. Provider with `BLOCKED_ENV` but no `blocked_reason` → FAIL.
4. Provider with `contract_status: "NOT_IMPLEMENTED"` but `runtime_status: "RUNTIME_PASS"` → FAIL.
5. Provider with `environment_kind: "simulated"` but `runtime_status: "RUNTIME_PASS"` → FAIL.
6. Matrix with `verified_providers` count mismatch → FAIL.

## Relationships to Other Schemas

- **p2-evidence-v8**: Provider evidence uses same status vocabulary.
- **p2-inspector-v8**: Inspector snapshot includes provider info.
- **p2-decision-ledger-v8**: Provider decisions recorded in ledger.
- **p2-extension-decision-ledger-v8**: Extension providers have additional decision types.

## Provider Evidence Requirements

Each `RUNTIME_PASS` provider must have:

1. Evidence JSON file at `artifacts/p2/evidence/<provider-kind>.json`.
2. Evidence file follows `p2-evidence-v8` schema.
3. Evidence includes real environment verification proof.
4. Evidence includes Inspector snapshot reference.
5. Evidence includes DecisionLedger reference.
6. Evidence includes anti-fake compliance proof.

## Provider Limitations Documentation

Each provider entry can include limitations:

1. `limitation_type`: "environment", "feature", "protocol", "performance".
2. `description`: Human-readable limitation description.
3. `workaround`: Available workaround if any.
4. `impact`: Impact on user experience.

## Provider Commands Vocabulary

Each provider can support specific commands:

1. `probe`: Probe media information.
2. `open`: Open media source.
3. `read`: Read media data.
4. `seek`: Seek to position.
5. `close`: Close media source.
6. `list`: List directory contents.
7. `refresh`: Refresh credentials/tokens.
8. `health`: Check provider health.

## Matrix Update Rules

1. Matrix can only be regenerated with new evidence.
2. Status changes require new evidence file.
3. Blocked status can be updated with environment changes.
4. Provider additions require new task completion.
5. Provider removals require explicit justification.

## Final Gate Requirements

Before `PFFG-001` (Foundation Final Gate Reader):

1. All providers must have contract status.
2. All local providers must have runtime status.
3. Remote providers must have honest `BLOCKED_ENV` or `RUNTIME_PASS`.
4. Matrix must be generated and validated.

Before `APRV-001` (All Providers Runtime Verification Gate):

1. All providers must have `RUNTIME_PASS` status.
2. All evidence must be validated.
3. No `BLOCKED_ENV` or `CONTRACT_PASS` providers allowed.
4. Matrix must be complete and honest.