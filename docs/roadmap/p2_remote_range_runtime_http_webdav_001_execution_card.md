# P2 Remote Range Runtime HTTP WebDAV 001 Execution Card

Updated: 2026-06-27

Canonical task id: `KIVO-VIDEO-P2-REMOTE-RANGE-RUNTIME-FOUNDATION-HTTP-WEBDAV-001`.
Short task id: `REMOTE-RANGE-RUNTIME-HTTP-WEBDAV-001`.
Design freeze version: `V1.5-FINAL-R1`.
Evidence file: `artifacts/p2/evidence/REMOTE-RANGE-RUNTIME-HTTP-WEBDAV-001.json`.
Design baseline: `KIVO-VIDEO-P2-REMOTE-RANGE-RUNTIME-FOUNDATION-HTTP-WEBDAV-001 设计基线 V1.5-FINAL-R1 / ULTIMATE FREEZE`.

This card converts the V1.5-FINAL-R1 frozen design baseline into a directly executable assistant task. The DOCX at `C:\Users\Administrator\Downloads\KIVO-VIDEO-P2-REMOTE-RANGE-RUNTIME-FOUNDATION-HTTP-WEBDAV-001 设计基线 V1.5-FINAL-R1.docx` is the design authority for this task.

## Position

This task is the remote byte-range runtime foundation for HTTP Range and WebDAV Range. It is not an Emby, Alist, cloud-drive, HLS, FFmpeg, DirectPlay, DirectStream, cache, performance, or production Internet transport task.

Target runtime chain:

`remote HTTP/WebDAV source -> SourceOpenRequest -> remote range provider -> SourceSession(open) -> RangeProbe -> SourceReadRequest sequential/positioned -> HTTP/WebDAV byte range request -> SourceReadResult(real bytes) -> SourceSeekResult(cursor only) -> SourceEvidenceSnapshot(runtime_pass/runtime_failed) -> DirectPlayInput/DirectStreamInput source-access input`

Allowed final claims:

- `HTTP Range Runtime Foundation: RUNTIME_PASS`.
- `WebDAV Range Bridge: RUNTIME_PASS`.
- Remote range proof verified through real `206 Partial Content`.
- Remote read, positioned_read, seek, empty-source EOF, source_changed, redaction, DirectInput access_ref, stale-after-close, loopback socket runtime, EvidenceJsonValidator, malicious evidence rejection, and No Debug Dialog Gate verified.

Forbidden final claims:

- Emby, Alist, cloud drive, HLS, or all providers runtime verified.
- P2 complete.
- DirectPlay or DirectStream final decision verified.
- FFmpeg probe verified.
- high-bitrate streaming performance verified.
- production Internet HTTP runtime verified.
- NAS/WebDAV vendor compatibility verified.
- HTTP/2 multiplexing or HTTP/3 verified.
- non-range fallback playback verified.
- range proof runtime promotion implemented.
- Infuse/Kodi product-level parity or surpass claim.

Default acceptance level:

- `level_2_loopback_socket_fixture runtime verified`.
- `external network runtime not claimed`.
- `production HTTP transport complete not claimed`.
- `high bitrate playback performance not claimed`.
- `NAS/WebDAV vendor compatibility not claimed`.

If the implementation only uses deterministic in-memory transport, downgrade evidence to:

- `runtime_environment_kind = deterministic_transport_fixture_runtime`.
- `runtime_fixture_level = level_1_deterministic_transport_fixture`.
- `loopback_socket_runtime = not_verified`.

## Dependency Gate

Depends on:

- `SRC-CORE-SKELETON-001` complete or explicitly compatible equivalent.
- `LOCAL-FILE-SOURCE-RUNTIME-001` complete or explicitly compatible equivalent.
- Canonical Source Core contracts/runtime targets available.
- Local-file read/positioned_read/seek/close semantics already frozen.

STOP if:

- Source Core Skeleton is absent.
- local_file Source Runtime Foundation is absent.
- implementing this task would require redesigning the source-core skeleton beyond this task's minimal contract deltas.
- a provider/business shortcut would bypass the remote range foundation.

## Iron Gate Order

Follow the Kivo Video iron gate:

1. Concept.
2. Planning design.
3. Natural family tree.
4. Implementation.
5. Single file, single responsibility.
6. No flat files.

This task must not start implementation before the family tree, allowed-file boundary, and phase plan are written in the final report or working notes.

## Phase Plan

- Phase A: source_core contracts/runtime minimal additions.
- Phase B: HTTP parser, redaction, header, redirect, validator, transport DTO, and loopback fixture foundation.
- Phase C: HTTP range open/read/positioned_read/seek/close/session/direct_input plus HTTP tests.
- Phase D: WebDAV path, PROPFIND, resource-type, href matching, and HTTP Range bridge plus WebDAV tests.
- Phase E: Evidence JSON, EvidenceJsonValidator, malicious evidence tests, No Debug Dialog Gate, final report.

Each phase must run its minimum build/test set and report status. Final `RUNTIME_PASS` is allowed only after Phase A-E pass.

## RR-001 Branch / Clean / Inventory Gate

- Create or use an isolated branch named `kivo-video-p2-remote-range-runtime-http-webdav-001`.
- Do not work directly on `main` or `master`.
- Record current branch, HEAD, and `git status --short`.
- STOP on unrelated dirty workspace unless the user explicitly authorizes continuing.
- Inventory these roots before coding:
  - `backend/include/kivo/video/source_core`
  - `backend/src/source_core`
  - `backend/include/kivo/video/providers`
  - `backend/src/providers`
  - `backend/tests`
  - `backend/tools`
- Confirm no duplicate `SourceManager`, `ProviderManager`, `remote_source_all.cpp`, `http_all.cpp`, `webdav_all.cpp`, or giant provider file will be introduced.

## RR-002 Natural Family Tree Gate

Before implementation, write the natural family tree:

- `source_core/contracts`: remote range contracts, capability contracts, evidence contracts, access_ref contracts, opaque validator contracts, fallback candidate contracts, validator strength contracts.
- `source_core/runtime`: safe result helpers, safe evidence helpers, capability helpers, redaction self-check, zero-read/EOF helpers, access_ref resolution, checked arithmetic, safe `from_chars` parsing.
- `providers/http`: URI/redaction, header policy, redirect policy, Content-Range parser, validator, transport DTO/interface, connection policy, probe/profile, fallback candidate evidence, read, seek, close, session store, evidence, EvidenceJsonValidator.
- `providers/webdav`: path validation, request builder, PROPFIND probe, 207 href matching, resource type parser, range bridge, redaction, evidence.
- `tests/fixtures/http`: loopback server, route fixture, blocking/cancel fixture, connection reuse observation, concurrent read fixture, redirect chain fixture, sentinel secret capture.
- `tests/fixtures/webdav`: loopback WebDAV server, PROPFIND XML fixture, 207 Multi-Status fixture, collection/resource fixture, malformed security fixture, large XML body cap fixture.

## RR-003 Source Core Contract Delta Gate

Only make minimal additions. Do not rewrite the skeleton.

Add or confirm:

- Remote SourceErrorCode family: invalid URI/scheme/userinfo/fragment/host/port/header, WebDAV collection/PROPFIND errors, remote status/range/probe/content-range/body/validator/unknown-length/timeouts/cancel/auth/rate-limit/redirect/redaction/evidence/DNS/TLS/proxy/transport errors.
- `SourceCapabilityState { unsupported, supported, unknown }`; do not use naked bool for remote capability.
- `RangeProofKind { none, seekable_known_length, seekable_empty_source, range_observed_unknown_length_not_seekable, rejected_no_range, failed_protocol_error, failed_nonconformant_response }`.
- `RemoteFallbackCandidateKind` for no-range, unknown-length, tiny-probe/extended-probe outcomes, future non-range sequential, vendor quirk, auth/redirect policy, and future HLS/segment source. This is evidence only.
- `SourceAccessEligibility` with eligible seekable/empty and ineligible no-range, unknown-length, nonconformant, source_changed, closed, stale, auth failed, transport failed.
- `ValidatorStrengthLevel { strong_etag, last_modified_second_precision, weak_etag_hint, length_only_hint, none }`.
- `SourceCapabilitySnapshot` fields for tri-state capabilities, known length, opaque validator snapshots, validator strength, final URL binding, range proof kind, fallback kind, eligibility, proof generation, capability generation, and contract schema version.
- Empty source helper rules: `seekable_empty_source`, content length 0, eligible empty source, read EOF, seek only 0, DirectInput can be constructed as source-access input but not playable-media proof.

## RR-004 Evidence / AccessRef Contract Gate

Add or confirm remote evidence kinds and operations:

- Evidence kinds: provider runtime observed, range proof/rejected, ambiguous/nonconformant/extended probe, fallback candidate, validator observed/weak/rejected/precision/strength, unknown length, source changed, capability downgraded, URI/auth/header redacted, redirect observed, connection reuse observed, body cap observed, timeout/cancel, WebDAV collection/resource/malformed/href exact match, evidence JSON validated/schema failed.
- Evidence operations: open, head_probe, propfind_probe, range_probe, extended_range_probe, read, positioned_read, seek, close, direct_input, redirect, timeout, cancel, source_changed, connection_reuse, evidence_validation.
- `OpaqueSourceAccessRef` must contain provider kind, session id, contract schema version, and `remote_access_generation`.
- `AccessRefResolveResult` must cover valid, stale closed, stale generation mismatch, session not found, provider mismatch, contract mismatch, and source changed requires reopen.
- DirectInput consumers must only call public `resolve_access_ref`; they must not access provider session internals.

## RR-005 Cross-Layer Leak Ban Gate

No cross-layer contract object may contain:

- raw URL, full path, full query string, raw redirect target.
- Authorization, Cookie, signed URL query secret, username/password.
- raw ETag, raw Last-Modified.
- native socket/handle, transport-private object, connection pool raw key.
- raw ProviderInternalRemoteUri.
- raw WebDAV href/path.
- raw local filesystem path or UNC path.

## RR-006 Opaque Validator / SafeDigest Gate

- Implement `OpaqueRemoteValidator` as a copy/move/cross-thread-safe immutable snapshot with safe digests only.
- Raw strong ETag and raw Last-Modified remain provider-private and may be used for If-Range only inside provider-private code.
- Weak ETag is never used for If-Range; it may become an opaque best-effort changed hint.
- Length-only hint never proves source unchanged.
- Implement `SafeDigest` with digest scope and visible prefix length.
- Tests may use deterministic salt; runtime must use provider-scoped or app-install-scoped salt.
- Evidence JSON may output digest prefixes only.

## RR-007 Source Core Runtime Helper Gate

Allowed runtime helpers:

- remote read semantics comments.
- range proof evidence helper.
- fallback candidate evidence helper.
- safe error/evidence builder.
- DirectInput result access_ref resolve helper.
- redaction self-check helper.
- zero-read helper.
- known-length EOF helper.
- safe validator digest helper.
- opaque validator equality helper.
- capability generation bump helper.
- evidence schema validation result helper.
- safe `std::from_chars` parse helper.
- checked arithmetic and bounds-checked index helper.

Forbidden runtime dependencies:

- source_core/runtime must not include providers/http or providers/webdav.
- source_core/runtime must not parse HTTP status or WebDAV XML.
- source_core/runtime must not hold raw URL/header/token or manage connection pools.

## RR-008 HTTP Provider Family Gate

Implement semantic HTTP Range families, not flat manager files:

- `HttpRangeSourceProvider`
- `HttpRangeSessionStore`
- `HttpRangeSessionRecord`
- `HttpRangeProbe`
- `HttpRangeProbeStrategy`
- `HttpRangeProbeProfile`
- `HttpRangeRead`
- `HttpRangeSeek`
- `HttpRangeClose`
- `HttpRangeEvidence`
- `HttpRangeFallbackCandidateEvidence`
- `HttpRangeRedaction`
- `HttpRangeRedirectPolicy`
- `HttpRangeHeaderPolicy`
- `HttpRangeTransport`
- `HttpRangeTransportConnectionPolicy`
- `HttpRangeContentRangeParser`
- `HttpRemoteValidator`
- `HttpRangeEvidenceJsonValidator`

The HTTP provider must cover open, HEAD hint, GET Range `bytes=0-0`, `bytes=0-1`, optional extended bounded probe, 206/200/416 handling, unknown length, sequential/positioned/zero read, cursor-only seek, close tombstone, stale access_ref, DirectInput stale-after-close, source_changed, If-Range invalid 200 full-body rejection, timeout/cancel, auth/header/url redaction, redirect redaction/final binding/same-origin auth stripping, weak ETag rejection for If-Range, Last-Modified precision evidence, length-only hint, compressed/multipart rejection, body cap, connection reuse hook, auth isolation, concurrency policy, and evidence JSON validator.

Forbidden files:

- `http_provider_manager.cpp`
- `remote_source_manager.cpp`
- `http_all.cpp`
- one giant `http_remote_source.cpp`

## RR-009 WebDAV Provider Family Gate

WebDAV is a semantic layer over HTTP Range. It must not reimplement the byte-range core.

Implement:

- `WebDavSourceProvider`
- `WebDavPathValidation`
- `WebDavRequestBuilder`
- `WebDavPropfindProbe`
- `WebDavResourceTypeParser`
- `WebDavHrefMatcher`
- `WebDavRangeBridge`
- `WebDavRedaction`
- `WebDavEvidence`

WebDAV may do URL/path validation, PROPFIND Depth: 0 metadata hints, 207 Multi-Status href exact matching, HEAD hint, auth/header/path redaction, collection rejection, resource_type_unknown fallback, PROPFIND failure fallback, large XML body cap, safe XML parser policy, and bridge to HTTP Range read/seek/close/direct_input.

WebDAV must not define a second current_offset, range proof, positioned_read, seek semantics, evidence family, or HTTP Range parser.

## RR-010 Three-Layer Dependency Gate

Dependencies:

- source_core/contracts depends on neither runtime nor providers.
- source_core/runtime depends on contracts, not providers.
- providers/http depends on contracts and runtime, not WebDAV, Emby, Alist, cloud_drive, FFmpeg, UI, render, or decoder.
- providers/webdav depends on contracts/runtime and may depend only on the HTTP public range bridge interface, not HTTP private session internals.

Recommended CMake enforcement:

- `kivo_video_provider_http_range_public_bridge` exposes bridge DTO/interface only.
- providers/webdav links only the public bridge and does not include providers/http private headers.

## RR-011 Forbidden Scope Gate

This task must not implement or claim:

- Emby, Alist, cloud drive, HLS.
- FFmpeg decode/probe.
- render, D3D11, DXVA, Vulkan, WASAPI, UI, playlist, subtitle, metadata probe.
- final DirectPlay or DirectStream decision.
- cache/prefetch, adaptive bitrate, retry storm, provider manager, source manager.
- non-range sequential fallback implementation.
- production Internet HTTP transport, NAS vendor compatibility, high bitrate performance.

Especially forbidden:

- Treating HTTP 200, HEAD success, Content-Length, Accept-Ranges, cache hit, Weak ETag, or length-only hint as range proof.
- Using Weak ETag for If-Range.
- Letting redirect URL, Authorization/Cookie, signed URL query, raw ETag, raw Last-Modified, raw WebDAV href/path, compressed range, or multipart range cross into evidence/error/identity.
- Carrying auth across cross-origin redirect.
- Continuing after redaction self-check or evidence schema validation fails.
- Treating degraded fallback candidate as DirectInput eligible.

## RR-012 Range Proof Principle Gate

Seekable range proof must come from:

- GET Range probe.
- `206 Partial Content`.
- valid Content-Range.
- body length exactly matching Content-Range.
- parseable complete length, or explicit empty-source proof.
- range proof bound to final URL digest.

`200 OK + full body` may only produce fallback candidate evidence in strict range mode. It must not create a seekable session or successful read_at proof.

## RR-013 Range Probe Profile Gate

Default profile:

- GET Range `bytes=0-0`.
- fallback GET Range `bytes=0-1`.

Compatibility extended profile:

- optional GET Range `bytes=0-65535`, only after tiny probes are ambiguous or server-quirk candidates.
- must be bounded by response body cap.
- success requires 206, legal Content-Range, response_start 0, response_end <= request_end, exact body length, known total, body cap not hit, identity encoding, non-multipart content type, and final URL digest binding.
- success sets `range_proof_kind = seekable_known_length` and records extended probe evidence.
- failure does not create SourceSession and returns failed_with_evidence.

## RR-014 416 / Empty / One-Byte Gate

- `416 Content-Range: bytes */0` proves empty source and creates seekable empty session.
- `416 Content-Range: bytes */N`, N >= 2, may try `bytes=0-1` and optionally extended probe.
- `416 Content-Range: bytes */1` for `bytes=0-0` is nonconformant; do not fallback, do not extended-probe, do not create seekable proof.
- 416 without Content-Range is ambiguous; may attempt configured fallback but no proof until fallback succeeds.
- malformed Content-Range maps to `remote_content_range_invalid` and stops fallback.

## RR-015 Read / Seek / EOF Semantics Gate

Mirror local_file semantics:

- `offset == nullopt`: sequential read uses `current_offset` and updates it only after successful byte read.
- `offset.has_value()`: positioned read uses explicit offset and does not update `current_offset`.
- zero read sends no HTTP request, updates no offset, emits no read runtime evidence, and sets EOF by known length when available.
- known-length `read_start >= content_length` returns EOF without network access.
- seek is cursor-only and sends no network request.
- seek absolute target equal to content length is allowed; target greater than length fails.
- empty source read always EOF; seek only 0 succeeds.

## RR-016 Strict Range / Future Fallback Boundary Gate

Implement only `strict_range_required`.

Do not implement in this task:

- 200 full stream fallback.
- full body skip-prefix simulated positioned_read.
- full body cache-backed read_at.
- non-range DirectInput eligibility.
- degraded fallback SourceSession.
- runtime proof promotion from fallback to seekable.

Future fallback work belongs to `KIVO-VIDEO-P2-NON-RANGE-SEQUENTIAL-FALLBACK-011` or `KIVO-VIDEO-P2-RANGE-PROOF-PROMOTION-POLICY-015`.

## RR-017 Transport / Fixture Gate

Define `HttpRangeTransport` with non-throwing `head`, `get`, and `propfind` request methods. Providers/http must not parse WebDAV XML; `propfind` exists only because WebDAV uses HTTP as transport.

Testing must use:

- `LoopbackHttpFixtureServer`
- `LoopbackWebDavFixtureServer`
- optional `DeterministicTransportFixture`

If only deterministic fixture is implemented, evidence must downgrade to Level 1. Do not claim production HTTP, Internet, DNS, TLS, proxy, NAS/WebDAV vendor compatibility, or high bitrate performance.

## RR-018 Connection Reuse / Same-Origin / Connection Key Gate

Implement policy/DTO hooks for:

- keep-alive connection reuse and observation.
- per-origin connection key.
- idle timeout.
- max connections per origin.
- max concurrent requests per origin.
- max concurrent requests per session.
- future prefetch stats and retry storm prevention hooks.

Same-origin is exactly normalized scheme + normalized host + normalized port. Subdomain, port change, scheme change, and downgrade are cross-origin.

Connection key must include scheme, host, port, and auth isolation class digest. Raw auth/query/token must not appear in key evidence. Cross-origin redirect strips auth before request and switches to anonymous auth isolation.

## RR-019 HTTP Request / Response DTO Gate

`HttpRangeRequest` separates provider-private and safe fields:

- safe request id.
- method.
- provider-private `ProviderInternalRemoteUri`.
- safe `RedactedSourceUri`.
- provider-private headers.
- optional byte range.
- timeout and cancel token.
- response body cap.
- connection reuse policy.

Default header: `Accept-Encoding: identity`.

Reject:

- gzip/br/deflate or other non-identity range responses.
- multipart/byteranges.
- CRLF injection, obs-fold, external Range/Host/Accept-Encoding override, duplicate sensitive header confusion, excessive header size/count.

`HttpRangeResponse` must cap body, separate safe/private headers, carry parsed Content-Range, opaque/private validators, redirect digest, transport result, connection reuse observation, and `body_cap_hit`.

If `body_cap_hit` is true, do not continue ordinary body-length mismatch validation.

## RR-020 HTTP Open Gate

Input:

- `SourceOpenRequest` provider kind http.
- raw URI consumed only at open boundary.
- optional auth secret ref.
- timeout/cancel.
- strict range mode.
- optional redirect policy.
- default or extended range probe profile.

URL validation:

- accept `http://` and `https://`.
- reject file/ftp/empty/relative/NUL/userinfo/nonempty fragment/invalid host/invalid port/empty host/non-ASCII host/IDN not handled.
- fragment is rejected because it can contain sensitive data.

Open flow:

1. validate provider kind.
2. consume RawSourceUri only at open boundary.
3. parse and validate URL.
4. build redacted URI and identity digest.
5. build provider-private headers.
6. optional HEAD hint.
7. GET Range `bytes=0-0`.
8. fallback `bytes=0-1` when allowed.
9. optional extended bounded probe.
10. bind proof to final URL after redirect policy.
11. parse 206/200/416/auth/rate-limit/timeout/cancel.
12. build capability, fallback candidate, evidence.
13. register session only for `seekable_known_length` or `seekable_empty_source`.
14. return open result, including `failed_with_evidence` for safe failures.

## RR-021 Redirect Policy / Binding Gate

Define `RedirectChainDigest` with hop count, original/final URL digests, crossed_origin, downgraded_scheme, auth_stripped, and policy_limit.

Define injectable `HttpRedirectPolicy` with max redirects, http->https, https->http localhost/private-network allowances, cross-origin auth forwarding allowance, and final URL binding requirement.

Default:

- max redirects 5.
- allow http->https and same scheme.
- deny https->http except explicit localhost/private allowlist.
- deny cross-origin auth forwarding.
- require final URL bound to range proof.

Range proof and session identity must use final URL digest. Evidence can output only digest, hop count, origin/downgrade/auth-stripped booleans, and policy limit.

If read later redirects to a different final URL and policy disallows drift, return `remote_source_changed` or `remote_redirect_validator_mismatch`.

## RR-022 Content-Range Parser Gate

Implement independent parser for:

- `Content-Range: bytes <start>-<end>/<total>`
- `Content-Range: bytes <start>-<end>/*`
- `Content-Range: bytes */<total>`

Parser requirements:

- use `std::from_chars`.
- detect uint64 overflow.
- reject negative numbers, suffix range, multi-range, non-bytes units, and dangerous extra characters.

206 validation:

- unit bytes.
- response_start equals request_start.
- response_end >= response_start and <= request_end.
- `body_cap_hit == false`.
- body length exactly equals response range length.
- numeric total >= response_end + 1.
- `*` total only as unknown-length response, not seek proof.
- if session length known, total must match.
- encoding identity/absent and not multipart.

Errors map to precise remote error codes. Unknown length must not become seekable proof.

## RR-023 Session Record / Tombstone Gate

`HttpRangeSessionRecord` should keep:

- SourceSession.
- ProviderInternalRemoteUri and RedactedSourceUri.
- SourceIdentity.
- capability and evidence snapshots.
- current_offset and optional content_length snapshot.
- provider-private and opaque validators.
- remote range proof.
- auth ref and redirect digest.
- remote_access_generation and capability_generation.
- lifecycle state.
- connection reuse policy.

Provider-private record may temporarily hold raw URL, raw strong ETag, raw Last-Modified, and resolved auth headers, but never emits them to evidence/error/DirectInput/JSON/stdout/stderr/exception messages.

Close must call `tombstone_sensitive()` and best-effort secure-clear internal URI, private validator, and sensitive header map. Tests must verify the provider-owned secure clear path with observable test buffers without claiming allocator/OS absolute memory erasure.

In-flight close rules:

- mark session closing/closed immediately.
- new read/seek/direct_input fail immediately.
- issued transport requests receive cancel.
- after request returns, do not write raw validator, success evidence, or offset update into closed session.

## RR-024 Remote Validator Gate

Split validators:

- `ProviderPrivateRemoteValidator`: raw strong ETag, raw Last-Modified, final URL digest source, provider-private only, usable for If-Range.
- `OpaqueRemoteValidator`: digests, content length snapshot, final URL digest, provider kind, strength level, safe cross-layer snapshot.

Rules:

- Strong ETag may be used for If-Range.
- Weak ETag never goes into If-Range and never proves byte-range exactness.
- Last-Modified may be used when no strong ETag, but must record second-level precision limitation evidence.
- Content-Length only creates `length_only_hint`, never If-Range or unchanged proof.
- Source_changed triggers include validator change, total mismatch, range mismatch, final URL digest drift, conflicting 416, If-Range invalid 200, and 412.
- Source_changed must downgrade capability, increment capability and remote access generations, make DirectInput source-access ineligible, and emit runtime_failed evidence without raw validators or raw URL.

## RR-025 HTTP Read Gate

Preconditions:

- session open.
- lifecycle open.
- range proof exists.
- can_range_request/read supported.
- known content length available.
- eligibility is seekable or empty.

Range request:

- zero read exits before end calculation.
- enforce max_read_chunk_size.
- checked arithmetic for `start + requested_size - 1`.
- default max chunk 8 MiB; config may allow 1-32 MiB; P2 tests use small limits.

Response semantics:

- 206 validates Content-Range, body, validator; returns real bytes and runtime_pass evidence; updates offset only for sequential read; EOF based on total.
- 200 after proof means If-Range failed or range ignored; reject full body, prefer `remote_source_changed`, no success offset update.
- 416 with `start >= known_length` returns EOF ok; otherwise range unsatisfied or source_changed.
- map 401/403/407/412/429/503/5xx/DNS/TLS/proxy/timeout/cancel to precise errors.

## RR-026 HTTP Seek / Close Gate

Seek:

- no network request.
- requires open session, range proof, can_seek supported, known content length.
- supports absolute, from_end, relative_forward.
- allows target == content_length and from_end 0.
- rejects target > content_length and overflow.
- unknown length is not seekable in P2 strict mode.
- failure must not change offset.

Close:

- open close -> closed + tombstone.
- closed close -> ok.
- unknown close -> `session_not_found`.
- closed session read/seek/direct input fails.
- evidence_for closed session returns last safe snapshot.
- old access_ref resolves stale/session closed.

## RR-027 Auth / Header Redaction Gate

Use opaque `SourceAuthSecretRef` / `RemoteAuthRef`. Raw token/header/cookie must never enter contracts, evidence, errors, JSON, stdout, or stderr.

`HttpRangeHeaderPolicy` must:

- build private request headers.
- force `Accept-Encoding: identity`.
- reject CRLF injection, obs-fold, duplicate sensitive header confusion, external Range/Host/Accept-Encoding override.
- redact Authorization/Cookie/signed queries and sensitive cloud headers.
- provide safe User-Agent and default-no Referer policy.

Redirect rules:

- same-origin may retain Authorization/Cookie after redaction self-check.
- cross-origin strips Authorization/Cookie before request and uses anonymous connection key.
- https->http denied by default, except explicit localhost/private allowlist.
- redirect to non-http is rejected.

Every SourceError, SourceEvidence, RedactedSourceUri, DirectInput, Evidence JSON, and test stdout/stderr must pass redaction self-check and fail closed on sentinel leak.

## RR-028 Timeout / Cancel Gate

Every request has timeout. Interface should reserve connect, header, body, stall, and overall timeout even if P2 uses a combined `SourceTimeout`.

Timeout evidence:

- `timeout_observed`.
- timeout runtime_failed.
- `remote_timeout`, or more precise connect/stall timeout if available.

Cancel checks:

- before open.
- before request.
- before response header.
- during body read loop.
- mid-chunk.
- on session close.

Loopback fixture must prove body-block cancel returns `remote_cancelled`, does not wait for overall timeout, does not map to timeout, and does not update offset as success.

## RR-029 Rate Limit / Retry / Cache Boundary Gate

Recognize but do not retry-storm:

- 429 -> `remote_rate_limited`.
- 503/Retry-After -> `remote_temporary_unavailable`.
- safe retry_after_seconds hint only; no raw header.

Do not implement:

- automatic retry storm.
- exponential backoff manager.
- background retry.
- cross-provider retry manager.
- cache/prefetch.
- cache hit as range proof.
- 200 full body cache read_at simulation.

Only reserve connection reuse stats, future prefetch stats, and per-origin/per-session concurrency limits.

## RR-030 WebDAV Open / PROPFIND / Bridge Gate

WebDAV raw input is normally http/https URL. `webdav://` may be only redacted display or future input format, not required raw scheme.

PROPFIND:

- Depth: 0.
- request only current resource.
- body size cap.
- XML node count/nesting/text limits.
- no external entity or DTD.
- namespace-aware DAV:resourcetype parsing.
- supports 207 Multi-Status.
- exact href/resource matching after safe normalization.
- parent/sibling confusion rejected.
- href/path never enters evidence/log.

Resource type states:

- `collection_confirmed`: fail with `remote_collection_not_file`, no HTTP Range bridge.
- `file_like_confirmed`: may bridge to HTTP Range, but PROPFIND is only hint.
- `resource_type_unknown`: may fallback to HEAD + GET Range and records evidence.
- `malformed_security_risk`: fail closed with `remote_webdav_propfind_malformed`, no HTTP Range bridge.

405/501/403/property-missing harmless PROPFIND failures may fallback to HEAD + GET Range. XML security risk must fail closed.

## RR-031 Evidence Gate

HTTP open evidence must cover:

- contract_declared/open/contract_pass.
- uri/header/auth redacted.
- HEAD probe runtime pass/fail.
- range_probe runtime_pass.
- range_proof runtime_pass.
- validator observed/strength evidence.
- connection reuse observed when fixture proves it.
- extended probe runtime pass/fail.
- no-range runtime_failed range proof + runtime_pass range rejection + fallback candidate evidence.
- empty source proof.
- nonconformant one-byte 416 runtime_failed.
- weak ETag observed and rejected for If-Range.
- Last-Modified precision limitation.
- length-only validator hint.
- unknown-length observed but range proof runtime_failed.
- body cap observed.

HTTP read evidence must cover sequential read, positioned read, known-length EOF without network, observed 416 EOF, source_changed runtime_failed plus capability downgrade, If-Range invalid 200 full-body rejection, timeout, and cancel.

WebDAV evidence must reuse HTTP semantics while adding provider kind, collection/resource unknown, 207 href exact match, PROPFIND fallback, and malformed XML security rejection events.

## RR-032 DirectInput Gate

DirectInput must carry:

- SourceIdentity.
- SourceCapabilitySnapshot.
- SourceEvidenceSnapshot.
- RedactedSourceUri.
- OpaqueSourceAccessRef.
- safe metadata hint.
- contract schema version.
- SourceAccessEligibility.

DirectInput is only source-access input. It must not contain choose_direct_play, should_direct_stream, final decision, transcode, codec, container, or FFmpeg probe result.

Eligibility:

- known-length proof -> eligible seekable range source.
- empty source -> eligible empty source.
- no-range -> ineligible no-range with fallback candidate.
- unknown length -> ineligible unknown length.
- nonconformant -> ineligible nonconformant response.
- source_changed -> ineligible until reopen/revalidate.
- closed/stale -> ineligible session closed/stale reference.

## RR-033 Thread Safety Gate

Minimum basic thread-safety:

- HttpRangeSessionStore lookup is thread-safe.
- generation increments are thread-safe.
- close and read/seek/direct_input state updates are mutually safe.
- sequential read offset update is per-session locked.
- positioned read can run concurrently but checks lifecycle.
- evidence/capability/lifecycle/generation updates are thread-safe.
- rapid close during in-flight read is safe.
- source_changed vs direct_input is safe.

Allowed:

- different sessions concurrent read.
- same-session positioned read concurrency.
- same-session sequential read serialized.

P2 verifies no data race and no success write-back after close; it does not claim throughput performance or HTTP/2 multiplexing.

## RR-034 Loopback Fixture Gate

HTTP loopback fixture must support routes for:

- ok range, probe 0-1 fallback, extended probe, no-range 200, empty 416, range EOF 416, one-byte valid, one-byte nonconformant 416, unknown length, source changed, If-Range invalid 200, weak ETag, Last-Modified only, length-only, body cap, auth, redirect, redirect secret query, cross-origin redirect, loop/limit/mismatch, https->http localhost policy, timeout, cancel, cancel mid-chunk, rate limit, 503, compressed range, multipart range, bad Content-Range, keepalive, ambiguous probe.

WebDAV fixture must support loopback PROPFIND XML, 207 Multi-Status href cases, collection/resource-type cases, malformed security cases, large XML cap, and range bridge routes.

Fixtures must capture sentinel secrets and prove they do not leak.

## RR-035 Required HTTP Tests Gate

Add and run each executable directly:

- `http_range_open_runtime_test`
- `http_range_probe_runtime_test`
- `http_range_extended_probe_runtime_test`
- `http_range_read_runtime_test`
- `http_range_positioned_read_runtime_test`
- `http_range_concurrent_positioned_read_runtime_test`
- `http_range_seek_runtime_test`
- `http_range_zero_read_runtime_test`
- `http_range_416_runtime_test`
- `http_range_one_byte_variants_runtime_test`
- `http_range_no_range_runtime_test`
- `http_range_source_changed_runtime_test`
- `http_range_if_range_source_changed_200_runtime_test`
- `http_range_validator_runtime_test`
- `http_range_redaction_runtime_test`
- `http_range_auth_redaction_runtime_test`
- `http_range_redirect_runtime_test`
- `http_range_redirect_secret_query_redaction_test`
- `http_range_timeout_cancel_runtime_test`
- `http_range_cancel_mid_chunk_runtime_test`
- `http_range_rapid_close_inflight_read_test`
- `http_range_connection_reuse_runtime_test`
- `http_range_direct_input_runtime_test`
- `http_range_basic_thread_safety_test`
- `http_range_probe_ambiguous_runtime_test`
- `http_range_probe_nonconformant_runtime_test`
- `http_range_redirect_limit_runtime_test`
- `http_range_unknown_length_runtime_test`
- `http_range_access_ref_resolve_runtime_test`
- `http_range_connection_key_normalization_test`
- `http_range_connection_auth_isolation_test`
- `http_range_body_cap_runtime_test`
- `http_range_tombstone_sensitive_clear_test`
- `http_range_opaque_validator_runtime_test`
- `http_range_evidence_json_validator_test`

Failure style only:

- `std::cerr << "...";`
- `return 1;`

Do not use assert/throw/abort/terminate or other No Debug Dialog banned APIs in tests/helpers.

## RR-036 Required WebDAV Tests Gate

Add and run each executable directly:

- `webdav_open_runtime_test`
- `webdav_propfind_probe_runtime_test`
- `webdav_propfind_207_href_match_runtime_test`
- `webdav_collection_rejection_runtime_test`
- `webdav_resource_type_unknown_runtime_test`
- `webdav_propfind_malformed_runtime_test`
- `webdav_propfind_large_xml_cap_runtime_test`
- `webdav_propfind_fallback_runtime_test`
- `webdav_range_read_runtime_test`
- `webdav_positioned_read_runtime_test`
- `webdav_seek_runtime_test`
- `webdav_redaction_runtime_test`
- `webdav_auth_redaction_runtime_test`
- `webdav_direct_input_runtime_test`

Tests must prove PROPFIND is hint-only, 207 href exact-match, collection rejection, unknown-type fallback, malformed XML fail-closed, large XML body cap, 405/501/403 fallback, GET Range as proof, read/positioned_read/seek reuse HTTP semantics, auth/path redaction, and stale-after-close DirectInput.

## RR-037 Test Proof Requirements Gate

Tests must prove:

- real bytes: body `abcdef`, read 3 -> `abc`, positioned offset 4 size 1 -> `e`, positioned read does not alter sequential offset, seek absolute 2 then read 2 -> `cd`.
- range proof: GET Range 0-0/0-1/extended, 206, valid Content-Range, exact body length, range_proof runtime_pass, bound final_url_digest.
- empty source: 416 */0, seekable empty, read EOF, seek 0 ok, seek > 0 fail, DirectInput eligible_empty_source.
- one-byte valid vs nonconformant 416 */1.
- no-range 200 rejected as seekable with failed_with_evidence and no full body load.
- weak ETag never used for If-Range and never leaked.
- Last-Modified precision evidence.
- length-only hint not used for If-Range.
- unknown length not seekable and not DirectInput eligible.
- source_changed downgrades capability, bumps generations, stales access_ref, leaks no raw validator.
- If-Range invalid 200 full body rejected without success offset update.
- body cap returns remote_response_too_large or higher-priority context error, not body mismatch.
- auth and redirect redaction sentinel scans.
- same-origin/cross-origin auth stripping and connection key auth isolation.
- compressed and multipart ranges rejected.
- connection reuse observed without performance claim.
- connection key normalization.
- cancel, cancel mid-chunk, rapid close in-flight.
- tombstone clear observable path.
- OpaqueRemoteValidator safe equality/copy/move.
- WebDAV collection, 207 exact-match, unknown type fallback, malformed security risk, large XML cap.
- redirect limit.
- Evidence JSON malicious cases.

## RR-038 No Debug Dialog Gate

All new provider code/tests must avoid:

- `<cassert>`, `assert`, `abort`, `std::abort`, `std::terminate`.
- exception business control flow, `runtime_error`, `std::exception`.
- `.at()` on vector/map/unordered_map.
- `std::optional::value`.
- `std::stoull`, `std::stoi`, stringstream numeric parsing.
- `std::regex` in provider/parser business paths.
- `std::locale`, `std::codecvt`.
- unchecked `std::get<T>`.
- `dynamic_cast`.
- throwing JSON path.
- throwing filesystem overload for business errors.
- raw `new` in hot provider paths.

Use `std::from_chars`, explicit optional checks, bounds-checked helper plus `operator[]`, checked variant access, non-throwing JSON wrapper, checked arithmetic, and Result/expected-like error returns.

Add a remote-range grep gate such as `backend/tests/no_debug_dialog_gate_remote_range_test.cpp` covering provider HTTP/WebDAV roots and fixtures.

## RR-039 CMake Gate

Targets:

- `kivo_video_provider_http_range`
- `kivo_video_provider_http_range_public_bridge`
- `kivo_video_provider_webdav`

Each required HTTP/WebDAV test must have its own `add_executable`, own `add_test`, and be directly runnable. Optional all-tests runners and OBJECT test-common libraries may exist but must not replace individual tests.

Allowed links:

- HTTP range provider -> source_core contracts/runtime.
- HTTP range public bridge -> source_core contracts/runtime; bridge DTO/interface only.
- WebDAV provider -> source_core contracts/runtime + HTTP range public bridge.

Forbidden links:

- FFmpeg, renderer, decoder, UI, D3D11, WASAPI, provider_manager, source_manager, Emby, Alist, cloud_drive.

## RR-040 Evidence JSON Gate

Write `artifacts/p2/evidence/REMOTE-RANGE-RUNTIME-HTTP-WEBDAV-001.json`.

Core fields:

- `schema_version = p2-evidence-v15`.
- `design_freeze_version = V1.5-FINAL-R1`.
- `task_id = KIVO-VIDEO-P2-REMOTE-RANGE-RUNTIME-FOUNDATION-HTTP-WEBDAV-001`.
- `task_family = source_core`.
- `status = RUNTIME_PASS`.
- `runtime_verified = true`.
- `runtime_environment_kind = loopback_http_webdav_runtime`.
- `runtime_fixture_level = level_2_loopback_socket_fixture`.
- `provider_runtime_scope = http_range_and_webdav_range_only`.
- `contract_schema_version = 15`.

Implementation facts must cover Phase A-E, probe variants, 206 proof, parser/header limits, 200 rejection, fallback evidence, no degraded session/eligibility, 416/empty/one-byte/unknown-length/body-cap semantics, real bytes, positioned/sequential/seek/zero/eof, source_changed, access_ref, validator strength, weak ETag, Last-Modified, length-only, redaction, redirect policy, timeout/cancel, rate-limit, compressed/multipart/body-cap, connection reuse/key/auth isolation/concurrency, WebDAV PROPFIND/bridge/redaction, DirectInput, stale-after-close, thread-safety, EvidenceJsonValidator, malicious evidence rejection, and explicit not_implemented/not_claimed boundaries.

Anti-fake checks must assert no provider/source manager, no all-provider/P2/final decision claims, no raw URL/redirect/ETag/Last-Modified/auth/cookie/signed query/secret/blob/JWT/WebDAV path/Windows path/UNC/connection key in evidence/stdout/stderr, No Debug Dialog clean, and evidence validated by tool.

If only deterministic transport is used, downgrade runtime environment fields and mark loopback socket not verified.

## RR-041 EvidenceJsonValidator Gate

Add either:

- `backend/tools/evidence_json_validator/evidence_json_validator.cpp`

or:

- `backend/tests/evidence/evidence_json_validator_test.cpp`

Validate:

- schema_version, design_freeze_version, contract_schema_version, task_id.
- runtime_verified/status consistency.
- runtime_fixture_level/runtime_environment_kind consistency.
- all required implementation and anti_fake fields.
- all not_claimed/not_implemented boundaries.
- no sentinel secrets.
- no raw URL, raw redirect target, raw ETag, Last-Modified, Cookie, Authorization, connection pool key, full path/query, base64-like long blob, JWT-like token, signed query parameter, Windows local path, UNC path, or raw WebDAV href/path.

Failure returns `evidence_schema_validation_failed`, blocks `RUNTIME_PASS`, and fails CI.

## RR-042 Acceptance Wording Gate

The final report may say only the allowed runtime-foundation statements listed in this card. It must explicitly say:

- loopback runtime verified.
- external network runtime not claimed.
- production transport not claimed.
- Infuse/product-level playback parity not claimed.
- Infuse/product-level surpass not claimed.

It must not say P2 complete, all providers done, Emby/Alist runtime-ready, DirectPlay final done, production network done, or Infuse surpassed.

## RR-043 Frozen Implementation Order Gate

Follow this order unless a specific step is already satisfied and proven:

1. Branch/Clean/Inventory.
2. SourceErrorCode remote range additions.
3. SourceCapabilityState tri-state.
4. RangeProofKind and SourceAccessEligibility.
5. RemoteFallbackCandidateKind.
6. ValidatorStrengthLevel.
7. SourceEvidenceKind/Operation remote events.
8. AccessRefResolveResult and resolve_access_ref.
9. OpaqueRemoteValidator lifecycle/equality.
10. SafeDigest salt policy.
11. safe error/evidence builder.
12. capability generation bump helper.
13. HTTP redaction helper.
14. HTTP URL parser and validation.
15. ProviderInternalRemoteUri strong type and destructor secure_clear.
16. connection key normalization.
17. auth isolation digest helper.
18. HTTP request/response DTO.
19. body_cap_hit transport result.
20. HTTP header policy.
21. header parser limits and CRLF injection rejection.
22. Content-Range parser.
23. Content-Range overflow/mismatch tests.
24. validator parser with strong/weak ETag.
25. validator strength evidence.
26. Last-Modified precision evidence.
27. length-only validator hint.
28. User-Agent/Referer policy.
29. RedirectChainDigest.
30. RedirectPolicy injection.
31. redirect policy details.
32. transport interface with cancel hook.
33. connection reuse policy DTO.
34. concurrency policy DTO.
35. loopback HTTP fixture Level 2.
36. HTTP open strict range mode.
37. HEAD probe hint.
38. GET Range 0-0.
39. GET Range 0-1 fallback.
40. extended bounded probe.
41. bytes=0-1 one-byte handling.
42. 416 */1 nonconformant handling.
43. final URL binding.
44. 206 range proof.
45. unknown-length Content-Range.
46. response body cap.
47. body_cap_hit validation order.
48. Accept-Encoding identity and compressed rejection.
49. multipart rejection.
50. HTTP read real bytes.
51. positioned read offset rule.
52. concurrent positioned read.
53. sequential read offset rule.
54. zero read no request.
55. known-length EOF no request.
56. 416 EOF/empty/unsatisfied semantics.
57. no-range 200 rejection.
58. fallback candidate evidence.
59. If-Range invalid 200 source_changed.
60. seek cursor-only.
61. close/tombstone/access_ref stale.
62. private validator tombstone clear.
63. observable tombstone sensitive clear.
64. rapid close during in-flight read.
65. cancel mid-chunk.
66. source_changed validator, capability downgrade, generation bump.
67. weak ETag not used for If-Range.
68. auth/header/url redaction.
69. redirect redaction and cross-origin auth stripping.
70. redirect secret query redaction.
71. connection auth isolation.
72. timeout/cancel.
73. blocking fixture cancel simulation.
74. connection reuse observation.
75. rate-limit/status mapping.
76. HTTP DirectInput.
77. HTTP access_ref resolve.
78. HTTP basic thread-safety.
79. WebDAV path/redaction.
80. WebDAV PROPFIND Depth: 0 hint.
81. WebDAV 207 href exact-match.
82. WebDAV large XML cap.
83. WebDAV resource type four-state parser.
84. WebDAV PROPFIND 405/501/403 fallback.
85. WebDAV collection rejection.
86. WebDAV malformed security-risk fail-closed.
87. WebDAV bridge to HTTP Range read.
88. WebDAV tests.
89. CMake/CTest and shared test object library.
90. Evidence JSON.
91. EvidenceJsonValidator.
92. malicious Evidence JSON tests.
93. No Debug Dialog Gate.
94. Final Report.

## RR-044 Follow-On Boundary Gate

After this task, recommended next work:

1. `KIVO-VIDEO-P2-PRODUCTION-HTTP-TRANSPORT-WINHTTP-009`.
2. `KIVO-VIDEO-P2-REMOTE-AUTH-CREDENTIAL-MANAGER-002`.
3. `KIVO-VIDEO-P2-REMOTE-RETRY-POLICY-002`.
4. `KIVO-VIDEO-P2-SOURCE-PREFETCH-CACHE-003`.
5. Emby/Alist/cloud drive source access.
6. FFmpeg media probe.
7. DirectPlay/DirectStream decision.

Do not start Emby/Alist first if it would bypass the remote range foundation.

## RR-045 Final Report Gate

Final report must include:

- task id and design freeze version.
- branch, commit hash, remote push status, working tree status.
- files changed summary.
- allowed/forbidden files check.
- family tree confirmation.
- Phase A-E status.
- build commands.
- CTest commands.
- individual test list.
- evidence JSON path.
- EvidenceJsonValidator result.
- malicious Evidence JSON result.
- No Debug Dialog Gate result.
- redaction sentinel scan result.
- not-claimed boundaries.
- known limitations.
- next recommended task.

Do not claim commit/push unless it actually succeeded.
