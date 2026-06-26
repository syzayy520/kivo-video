# P2 Source Runtime Foundation Local File 001 Execution Card

Updated: 2026-06-27

Canonical task id: `KIVO-VIDEO-P2-SOURCE-RUNTIME-FOUNDATION-LOCAL-FILE-001`.
Short task id: `LOCAL-FILE-SOURCE-RUNTIME-001`.
Evidence file: `artifacts/p2/evidence/LOCAL-FILE-SOURCE-RUNTIME-001.json`.
Design baseline: `KIVO-VIDEO-P2-SOURCE-RUNTIME-FOUNDATION-LOCAL-FILE-001 设计基线 V1.4（执行冻结版）`.

This card converts the V1.4 frozen design baseline into a directly executable assistant task. The DOCX at `C:\Users\Administrator\Downloads\KIVO-VIDEO-P2-SOURCE-RUNTIME-FOUNDATION-LOCAL-FILE-001 设计基线 V1.4（执行冻结版）.docx` is the design authority for this task.

## Position

- This is Source Runtime Contract Freeze plus the first real `local_file` runtime proof chain.
- This is not a performance flagship freeze.
- This is not full player-core completion.
- This is not all providers runtime verified.
- This is not FFmpeg, decode, rendering, HDR, WASAPI, UI, or high-bitrate performance verification.
- This must run after `SRC-CORE-SKELETON-001` has created the canonical contracts/runtime skeleton.

Target runtime chain:

`real local file -> SourceOpenRequest -> local_file provider -> SourceSession(open) -> SourceReadRequest / SourceReadResult -> SourceSeekRequest / SourceSeekResult -> SourceCloseResult -> SourceEvidenceSnapshot(runtime observed) -> DirectPlayInput / DirectStreamInput source-access input`

Allowed final claim:

- `P2 Local File Source Runtime: RUNTIME_PASS`, only when all local-file runtime evidence gates pass.

Forbidden final claims:

- full P2 completion.
- all providers runtime verified.
- player core completion.
- final DirectPlay/DirectStream decision verification.
- FFmpeg/decode/render/HDR/WASAPI/performance verification.

## Dependency Gate

Depends on:

- `SRC-CORE-SKELETON-001` complete or explicitly compatible equivalent.
- Source Core contracts/runtime targets available.
- Canonical namespace and paths from skeleton task available.

STOP if:

- Source Core Skeleton is absent.
- canonical contracts/runtime paths are missing.
- local_file implementation would require redesigning source-core skeleton contracts beyond this task's allowed contract deltas.

## LF-001 Branch / Base / Clean Gate

- Confirm current branch.
- Confirm HEAD.
- Confirm `git status --short`.
- STOP on unrelated dirty workspace.
- STOP if Source Core Skeleton task is incomplete and the implementer cannot safely continue.
- Do not clean or delete user files without explicit instruction.

## LF-002 Schema Version Gate

- Use the repository-supported evidence schema.
- If the validator only supports `p2-evidence-v8`, use `schema_version = "p2-evidence-v8"`.
- Do not invent `p2-evidence-v9` unless a separate schema migration already exists and passes validation.
- Evidence JSON may include source contract schema version separately.
- STOP if schema validator rejects required fields and passing would require fake evidence.

## LF-003 RawSourceUri Open-Boundary Consumption Gate

The skeleton correctly keeps raw URI private. This task must add a restricted open-boundary consumption method so `local_file` can parse the input path.

Requirements:

- Add a strongly named method equivalent to `untrusted_input_for_open_boundary()`.
- Do not name the method as a generic raw value/path/url accessor.
- Limit use to local-file provider, local-file tests, and open-boundary contract test code if needed.
- Do not allow raw input to reach identity, evidence, error, redacted URI, DirectPlayInput, DirectStreamInput, artifacts, logs, stdout, or stderr.

## LF-004 Evidence Runtime Failure Semantics Gate

Required contract delta:

- Add runtime failure semantics to evidence pass kind.
- Runtime-observed failures such as source mutation must be recorded as runtime-failed evidence, not runtime-pass evidence.

Required event model:

- Keep internal evidence event identity as kind plus operation.
- JSON/report output may render a combined event label, but code should keep the two-layer structure.

Required operations include:

- open.
- read.
- positioned read.
- seek.
- close.
- direct input.
- source changed.

## LF-005 Positioned Read Semantics Gate

Freeze read offset semantics:

- `SourceReadRequest.offset == nullopt`: sequential read from current offset; successful read updates `current_offset`.
- `SourceReadRequest.offset has value`: positioned read from that offset; successful read does not update `current_offset`.

This is the cross-provider root semantic for future HTTP, WebDAV, Emby, AList, and similar providers.

## LF-006 Cross-Provider Stable Error Semantics Gate

Extend source-core contract errors as needed so local-file runtime and future providers share common meanings.

Required coverage:

- missing or invalid sessions.
- closed sessions.
- stale source access references.
- invalid requests.
- read-size guard failures.
- unsupported provider/path cases.
- source mutation/unavailability.
- seek unsupported/failed.
- open/read/close failures.
- provider runtime errors.

Rules:

- Errors must be defined by source-core contracts.
- Providers must not privately redefine public source error meanings.
- `source_changed` must not be reported as EOF.
- unsupported must not be reported as success.

## LF-007 SourceSession Lifecycle Gate

Public session states stay:

- `created`.
- `open`.
- `closed`.
- `error`.

Rules:

- Successful open returns `SourceOpenResult::success(SourceSession{state=open})`.
- Failed open returns `SourceOpenResult::failure(SourceError)` and no half-initialized session.
- Closed sessions cannot read, seek, or build direct input.
- `SourceSession` may expose session id, state, contract schema version, and optionally provider kind.
- `SourceSession` must not store file objects, paths, native handles, provider native objects, tokens, headers, cookies, or credential material.

## LF-008 OpaqueSourceAccessRef Gate

Add an opaque source-access reference for DirectInput handoff.

Required fields:

- `SourceSessionId`.
- provider kind.
- contract schema version.

Rules:

- No raw path.
- No native handle.
- No token or credential.
- Does not extend session lifetime.
- Must be resolved through provider/session store each time.
- Close makes the ref stale.
- Stale refs return closed/stale-reference errors.
- DirectInput holding this ref does not equal owning file access.

## LF-009 Local File Family Structure Gate

Allowed family roots:

- `backend/include/kivo/video/providers/local_file/**`
- `backend/src/providers/local_file/**`
- `backend/tests/providers/local_file/**`

Recommended responsibilities:

- local-file source provider.
- session record and session store.
- read operation.
- seek operation.
- close operation.
- evidence builder.
- redaction.
- path validation.
- mutation guard.
- I/O strategy.
- opaque URI digest.

Do not add provider managers, source managers, all-in-one runtime files, remote providers, or final decision engines.

## LF-010 LocalFileSourceProvider Gate

Recommended public provider surface:

- open.
- read.
- seek.
- close.
- evidence snapshot retrieval.
- make DirectPlayInput source-access candidate.
- make DirectStreamInput source-access candidate.

Rules:

- Business errors use typed result objects, not exceptions.
- File-system APIs must use no-throw/error-code overloads where practical.
- Do not force `noexcept` on functions that may allocate strings/vectors.
- Out-of-memory behavior is not a business-error guarantee for this task.

## LF-011 LocalFileSessionStore Gate

Responsibilities:

- allocate `SourceSessionId`.
- store local-file session records.
- find records.
- update current offset.
- append evidence.
- close sessions.
- preserve closed tombstones.
- reject read/seek on closed sessions.
- reject direct input for closed sessions.
- prevent shared file pointer race.

Record-private data may include provider-internal safe path and observed file identity, but this data must not leak into public identity, evidence, errors, direct input, logs, or redacted output.

Thread-safety scope:

- V1.4 recommends a mutex-protected basic thread-safety boundary.
- You may claim basic boundary verified only if tests prove it.
- Do not claim concurrent performance verified.

## LF-012 I/O Strategy Gate

The contract does not bind a specific I/O strategy.

Allowed implementation approaches:

- temporary open per read proof.
- lazy/eager persistent provider-private handle.
- platform-specific read-at strategy only if still isolated inside provider internals.

Rules:

- Native handles never enter public SourceSession, evidence, DirectInput, or errors.
- Close and destruction release provider-private resources.
- Close must be idempotent.
- Positioned read must not rely on unsafe shared file pointer behavior.
- Persistent handle, if used, must be a provider-private RAII object.

## LF-013 Tombstone Close Gate

Close behavior:

- close(open session): block future read/seek, release provider-private resources, mark state closed.
- close(closed session): return closed success without error.
- close(unknown session): return session-not-found error.
- closed tombstone remains for evidence snapshot and stale-ref behavior.
- No tombstone GC in this task.

## LF-014 Local File Open Gate

Accepted input:

- absolute native local path.
- file URI that resolves to an absolute local path.

Default rejects:

- relative path.
- empty path.
- path containing NUL.
- symlink.
- non-regular file.
- directory.

UNC/network path handling:

- either explicitly reject/defer to future network providers, or document an accepted local-file policy if project requirements force it.

Open sequence:

- verify provider kind is local_file.
- consume raw input only through open-boundary method.
- validate path with no-throw file-system APIs.
- collect file size and last-write snapshot where available.
- create opaque digest.
- create redacted URI.
- create SourceIdentity.
- create SourceCapabilitySnapshot.
- append runtime-observed open evidence.
- register session record.
- return successful open session.

## LF-015 Opaque Digest / Redacted URI Gate

Digest input should include:

- provider kind.
- normalized absolute path used internally only.
- file size snapshot.
- last-write snapshot if available.
- digest salt/version.

Rules:

- Full digest may be internal/opaque identity material.
- Redacted URI may use a short digest prefix and file extension.
- Do not claim collision impossibility.
- Do not output raw path, username path, filename stem, symlink target, native handle, or original raw input.

Default redacted shape:

- `local-file://sha256-12:<digest-prefix>.<ext>`

## LF-016 Local File Read Gate

Sequential read:

- offset absent.
- read from current offset.
- successful read updates current offset.

Positioned read:

- offset present.
- read from requested offset.
- successful read does not update current offset.

Zero-size read:

- does not access file.
- does not update offset.
- does not append read runtime evidence.
- returns EOF according to current offset and file size semantics.

EOF/over-read:

- read start equal to file size returns EOF with no error.
- read start greater than file size returns EOF with no error.
- source mutation/unavailability must be reported as an error, not EOF.

Required data test:

- create known content such as `abcdef`.
- read first 3 bytes as `abc`.
- seek absolute 2, then read 2 bytes as `cd`.
- positioned read offset 4 size 1 as `e`.
- prove positioned read did not change current offset.

Safety rules:

- enforce maximum read chunk size.
- check arithmetic overflow before size/offset addition.
- compute safe read size before resizing buffers.
- avoid debug-dialog-prone unchecked access.

## LF-017 Local File Seek Gate

Required seek kinds:

- absolute.
- from end.
- relative forward if implemented.

Rules:

- absolute target becomes new position if within file size.
- from-end target subtracts from file size and fails if beyond file size.
- seeking to file size is allowed.
- seeking beyond file size fails.
- read over EOF is EOF, but seek over EOF is an error.

## LF-018 Source Mutation Gate

At open, record:

- file size snapshot.
- last-write snapshot where available.
- optional platform file id where safely available.

At read, best-effort detect:

- path missing.
- no longer regular file.
- file size changed.
- last-write changed if recorded.
- platform file id changed if implemented.
- unexpected short read outside valid EOF conditions.

Detected source change:

- evidence kind is source-changed observed.
- operation is source changed.
- pass kind is runtime failed.
- never report as runtime pass.

## LF-019 Capability Gate

On successful local-file open:

- can read is supported.
- can seek is supported.
- range-like byte access is supported for source-byte range semantics.
- known content length is file-size snapshot.
- content-length semantics are open-time snapshot.
- MIME type remains unknown unless safely known.
- container hint is absent or safe extension hint.
- direct-play candidate remains unknown.
- direct-stream candidate remains unknown.
- performance profile remains unspecified.

Do not convert readable local source into a playback/direct-play success claim.

## LF-020 Evidence Gate

Local-file runtime may produce runtime pass evidence only for observed local-file operations.

Expected evidence:

- contract declared.
- URI redacted.
- identity constructed.
- capability declared.
- provider runtime observed open.
- provider runtime observed read.
- provider runtime observed positioned read.
- range proof seek.
- session closed.
- source changed runtime failed.

Rules:

- zero-size read does not append read runtime-pass evidence.
- sequence number monotonic inside a session.
- evidence must not include raw path, raw input, tokens, credentials, native handles, username path, absolute drive path, filename stem, or symlink target.

## LF-021 DirectInput Gate

DirectPlayInput and DirectStreamInput are source-access candidate inputs only.

Recommended content:

- SourceIdentity.
- SourceCapabilitySnapshot.
- SourceEvidenceSnapshot.
- RedactedSourceUri.
- OpaqueSourceAccessRef or SourceSessionId.
- safe metadata hint.
- contract schema version.
- evidence completeness hint.

Failure cases:

- unknown session.
- closed session.
- error session.
- stale reference.
- redaction failure.
- missing minimum open runtime evidence.

Forbidden:

- final playback decisions.
- transcode decisions.
- FFmpeg/container/codec validation.
- performance decisions.

## LF-022 Schema / Evidence JSON Gate

Required evidence file:

- `artifacts/p2/evidence/LOCAL-FILE-SOURCE-RUNTIME-001.json`

Required top-level facts:

- `schema_version` follows repository validator support, normally `p2-evidence-v8`.
- `source_contract_schema_version` records source contract schema version separately.
- `task_id` is `KIVO-VIDEO-P2-SOURCE-RUNTIME-FOUNDATION-LOCAL-FILE-001`.
- `task_family` is `source_core`.
- `status` is `RUNTIME_PASS` only if all local-file runtime gates pass.
- `runtime_verified` is true only after real temp-file runtime tests pass.
- `runtime_environment_kind` is `local_file_temp_runtime`.
- provider is `local_file`.
- provider runtime scope is `local_file_only`.

Required implementation facts:

- open verified.
- sequential read verified.
- positioned read verified.
- zero read verified.
- over-read EOF verified.
- seek verified.
- close verified.
- tombstone close verified.
- source access ref lifetime verified.
- runtime pass evidence verified.
- runtime failed evidence semantics verified.
- DirectPlayInput and DirectStreamInput constructed as source-access candidates.
- minimum evidence checked.
- raw path, filename stem, and native handle redaction verified.
- max read chunk guard verified.
- raw URI open-boundary consumption verified.
- digest uses path/size/mtime internally.
- remote provider runtime not implemented.
- performance benchmark not claimed.

Anti-fake facts:

- no provider-manager/source-manager implementation.
- no remote-provider runtime claim.
- no all-providers runtime verified claim.
- no P2 complete claim.
- no FFmpeg/decode/render/performance claim.
- no raw path in evidence JSON.

## LF-023 Runtime Test Matrix Gate

Register CTest coverage for:

- open runtime.
- read runtime.
- zero read runtime.
- seek runtime.
- error runtime.
- close runtime.
- redaction runtime.
- evidence runtime.
- direct input runtime.
- path validation runtime.
- schema runtime.
- basic thread-safety boundary.

Required behavior coverage:

- successful and failed open.
- absolute path, relative path, directory, symlink.
- sequential read.
- positioned read without offset update.
- zero read without real I/O or read evidence.
- over-read EOF.
- max read chunk guard.
- seek absolute, from end, file size, beyond file size.
- idempotent close and tombstone.
- closed session read/seek/direct input failure.
- redaction does not leak raw path, filename stem, username, drive path, or raw input.
- runtime pass/runtime failed evidence distinction.
- stale source access ref.
- schema version consistency.
- source mutation best-effort.
- basic thread-safety boundary if implemented.

## LF-024 No Debug Dialog Gate

New tests and code must not rely on debug-runtime abort behavior.

Rules:

- Do not use assertion/abort patterns that trigger MSVC Debug Error popups.
- Tests should report errors and return nonzero.
- Run each new test executable directly at least once, not only via build.
- Avoid unchecked vector access patterns called out in the DOCX baseline.

## LF-025 CMake Gate

Required provider target:

- `kivo_video_provider_local_file`

Required test executable names:

- `kivo_video_provider_local_file_open_test`
- `kivo_video_provider_local_file_read_test`
- `kivo_video_provider_local_file_zero_read_test`
- `kivo_video_provider_local_file_seek_test`
- `kivo_video_provider_local_file_error_test`
- `kivo_video_provider_local_file_close_test`
- `kivo_video_provider_local_file_redaction_test`
- `kivo_video_provider_local_file_evidence_test`
- `kivo_video_provider_local_file_direct_input_test`
- `kivo_video_provider_local_file_path_validation_test`
- `kivo_video_provider_local_file_schema_test`
- `kivo_video_provider_local_file_basic_thread_safety_test`

Dependency rules:

- local-file provider target links source-core contracts and runtime.
- tests link local-file provider target.
- do not link media pipeline, device/audio/graphics subsystems, or provider-management layers.

## LF-026 Execution Order

Execute in this order:

1. Source error code minimal extension.
2. Evidence kind plus operation structure.
3. Evidence pass kind runtime-failed semantics.
4. Schema version consistency confirmation.
5. Raw input open-boundary consumption method.
6. close/direct input result types.
7. Opaque source access ref.
8. local-file redaction.
9. local-file path validation.
10. opaque digest.
11. session store and tombstones.
12. I/O strategy.
13. local-file provider open.
14. open runtime tests.
15. read semantics and tests.
16. seek semantics and tests.
17. close semantics and tests.
18. evidence append and sequence numbers.
19. redaction tests.
20. DirectInput construction and stale-ref tests.
21. schema consistency test.
22. source mutation test.
23. basic thread-safety test.
24. CMake/CTest.
25. evidence JSON.
26. No Debug Dialog Gate.

## LF-027 Final Report Gate

Final report must include:

- classification.
- branch and HEAD.
- whether Source Core Skeleton dependency was satisfied.
- changed files by family.
- SourceSession lifecycle proof.
- open/read/positioned-read/seek/close proof.
- zero read and over-read EOF proof.
- source mutation runtime-failed proof.
- redaction proof.
- evidence proof.
- DirectInput source-access carrier proof.
- stale-after-close proof.
- basic thread-safety boundary proof if implemented.
- CMake target proof.
- direct test-executable run proof.
- CTest proof.
- schema validation proof.
- no-debug-dialog proof.
- forbidden-scope proof.
- final evidence JSON path.
- known limitations.
- final recommendation.

Required final statement:

```text
This verifies local_file Source Runtime Foundation only.
This does NOT verify all providers.
This does NOT complete P2.
This does NOT verify DirectPlay or DirectStream final decisions.
This does NOT verify FFmpeg, decode, rendering, HDR, WASAPI, or high-bitrate performance.
```
