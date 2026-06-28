# P3 Complete Line V9 Closure-Ready Execution Card

Updated: 2026-06-27

Canonical planning task id: `KIVO-VIDEO-P3-COMPLETE-LINE-DESIGN-PLANNING-V9`.
Short task id: `P3-COMPLETE-LINE-V9`.
Source document: `Kivo Video P3 完整线路设计规划 v9 Closure-Ready Ultimate`.
Source DOCX: `C:\Users\Administrator\Downloads\Kivo Video P3 完整线路设计规划 v9 Closure-Ready Ultimate.docx`.

This execution card converts the P3 v9 complete-line planning document into numbered gates for another assistant to execute. It is intentionally explicit because P3 is broad and easy to confuse with P2 continuation or playback-core implementation.

## Core Position

P3 is not more P2.

P3 is not the renderer, decoder, audio output, UI, player timeline, segment scheduler, or playback core.

P3 is the production bridge from Source Runtime to Playback-Ready Input System:

`Source Runtime -> Provider / Transport / Delivery / Probe / Decision / Handoff / Evidence / Security -> Playback-ready input candidates for P4`

P3 upgrades P2's "can read remote bytes" into:

`safe, low-latency, evidence-backed, cancellable, async, layered, rankable, runtime-revisable, secure handoff-ready playback input`.

## Dependency Order

Do not execute P3 gates until the P2 source runtime final closure is complete:

1. `SRC-CORE-SKELETON-001`
2. `LOCAL-FILE-SOURCE-RUNTIME-001`
3. `REMOTE-RANGE-RUNTIME-HTTP-WEBDAV-001`
4. `P2-SOURCE-RUNTIME-FINAL-CLOSURE-001`
5. `P3-COMPLETE-LINE-V9`

STOP if P2 is not closed and source runtime baseline is not locked.

## Global Rules

Evidence must be machine-readable where possible and supported by test artifacts. Human prose alone is not enough for closure.

All gates must follow the Kivo iron gate:

1. Concept.
2. Planning design.
3. Natural family tree.
4. Implementation.
5. Single file, single responsibility.
6. No flat files.

No gate may introduce broad root-level files, manager catchalls, god objects, or "everything" modules.

## Global Allowed / Forbidden Scope

Allowed P3 outputs:

- ExecutionContext, PlaybackLatencyBudget, FastStartPath, PreconnectPolicy.
- PlatformCapabilityHints, DecoderCapabilityRequirement, AudioCapabilityRequirement, SubtitleCapabilityRequirement, BufferRequirement.
- SourceOriginIdentity, ProviderIdentity, SourceTransportIdentity, MediaDeliveryIdentity, SourceIdentity, SourceCapability.
- SourceSessionPlan, ManifestSessionPlan, PlaylistSessionPlan.
- SourceSession, IndependentRangeReader, ManifestSession, PlaylistSession.
- UnsupportedSourceSignal.
- MediaProbePlan, FastStartEvidence, ProbeEvidence, ManifestEvidence, PlaylistEvidence.
- PlaybackCandidateSet, CandidateRanking, PairwiseCandidateComparison, PlaybackDecisionTrace.
- PlaybackHandoffContract, PlaybackDecisionRuntime, PlaybackDecisionRevision, RuntimeRevisionResult.
- DirectPlayInputCandidate, DirectStreamInputCandidate, HlsInputCandidate, DashInputCandidate, PlaylistInputCandidate.
- NotPlayableReason, ProbeRequiredReason, RetryableNetworkSignal, SessionErrorSignal, CancellationEvidence, RedactedEvidence.
- ExecutionCard and ClosureEvidence.

Forbidden P3 outputs:

- decoded frame.
- rendered frame.
- audio sample output.
- subtitle bitmap.
- UI state.
- playback timeline control.
- real transcoded media.
- persistent media cache artifact.
- DRM decrypted sample.
- HLS segment playback queue.
- DASH segment playback queue.
- platform media session event execution.
- AirPlay playback session.
- Chromecast playback session.

## Milestone Model

P3 must be executed as maturity layers, not as a single mixed task:

- P3-M0: Contract + Guard + Product Baseline.
- P3-M1: Fake Provider + Fast Start Integration Baseline.
- P3-M1.5: Runtime Ownership + Security Hardening Baseline.
- P3-M2: Production Hardening Closure.

## FC-001 P3 Identity Gate

Goal: Lock P3 meaning before any implementation.

Evidence target:

- `docs/roadmap/p3_complete_line_v9_closure_ready_execution_card.md`
- future closure evidence field `p3_positioning_verified = true`

Allowed:

- State P3 as Source Runtime to Playback-Ready Input System bridge.
- Reference P2 local_file, HTTP range, WebDAV range as inputs.
- Define P3 input/output boundaries.

Forbidden:

- Calling P3 "P2 continuation".
- Calling P3 "playback core".
- Claiming decoder/render/audio/UI completion.

STOP if:

- any task names P3 work as a P2 child task.

## FC-002 P2 Closure Dependency Gate

Goal: Ensure P3 starts only after P2 source runtime closure.

Evidence target:

- `artifacts/p2/evidence/P2-SOURCE-RUNTIME-FINAL-CLOSURE-001.json`
- `docs/execution_cards/P2_SOURCE_RUNTIME_FINAL_CLOSURE_001.md`
- P3 closure report section `p2_closure_dependency`

Allowed:

- Read P2 closure evidence/card.
- Record P2 closure commit and locked baseline.

Forbidden:

- Starting P3 implementation if P2 closure is absent.
- Using P3 to repair unfinished P2 source runtime gates.

STOP if:

- P2 final closure evidence/card is missing, invalid, or still in TODO state.

## FC-003 Scope Level Gate

Goal: Split P3 work into blocking implementation, fake-provider foundation, contract-only boundary, and explicitly not implemented.

Evidence target:

- `artifacts/p3/evidence/p3_scope_level_matrix.json`
- closure report section `scope_boundary`

Allowed:

- Mark ExecutionContext, sessions, provider integration, network policy, probe, decision, handoff, golden evidence, and closure as blocking.
- Mark fake Emby/Jellyfin/Alist/WebDAV/HTTP/HLS/DASH/playlist as fake-provider foundation.
- Mark CloudDrive, SMB, UPnP/DLNA, FTP/RTSP/RTMP, DRM, subtitle download, audio hints, platform hints, library scraper as contract-only or unsupported as specified.

Forbidden:

- Treating all items as mandatory production implementations.
- Treating contract-only items as fake PASS.
- Using real accounts as closure blockers.

STOP if:

- any scope item lacks a level assignment.

## FC-004 Explicit Not Implemented Gate

Goal: Prevent P3 from silently becoming playback core.

Evidence target:

- `artifacts/p3/evidence/p3_not_implemented_boundary.json`

Allowed:

- Define contracts and evidence for later stages.
- Produce candidates and handoff contracts for P4.

Forbidden:

- UI or Qt pages.
- video rendering, audio output, WASAPI, D3D11/DXGI/Vulkan execution.
- HDR tone mapping implementation.
- subtitle rendering.
- complete decode/demux pipeline.
- transcoding.
- HLS/DASH segment playback queues.
- real ABR scheduling.
- persistent media cache or prefetch cache.
- DRM license acquisition or decryption.
- full scraper clients.
- AirPlay/Chromecast/DLNA renderer playback.

STOP if:

- a P3 module imports or links to render/audio/UI/decode implementation.

## FC-005 V9 Critical Fix Gate

Goal: Ensure all v9 critical fixes are represented in gates/tests.

Evidence target:

- `artifacts/p3/evidence/p3_v9_critical_fix_matrix.json`

Allowed:

- Track each critical item to a contract, test, and evidence artifact.

Forbidden:

- Leaving any critical item as "documentation only".
- Closing P3 without mapping critical items.

STOP if any missing:

- ManifestSessionLease and RuntimeRevision ownership rule.
- snapshot supersede policy.
- IndependentRangeReader observer closure after P4 handoff.
- DNS rebinding CNAME/redirect/connection-pool checks.
- FastStartEvidence confidence merge through CriticalConstraintsChecklist.
- background ProbeEvidence downgrade forcing RuntimeRevision.
- SecureMemory best_effort credential blocking.
- ExecutionContext policy propagation.
- provider metadata skip-probe cross-validation.
- CandidateRanking no reverse provider dependency.
- closure evidence no current commit hash dependency.
- no real account dependency.
- Windows primary platform hints only, no cross-platform API calls.

## FC-006 V9 High-Priority Convergence Gate

Goal: Capture v9 high-priority convergence items.

Evidence target:

- `artifacts/p3/evidence/p3_v9_convergence_matrix.json`

Allowed:

- Add tests and architecture policy for convergence items.

Forbidden:

- Encoding optional fields without default/when-present semantics.
- Omitting accepted deferrals from closure report.

STOP if missing:

- provider fast metadata/probe boundary.
- CandidateRanking/SourceCapability no cyclic dependency.
- simple playlist vs HLS M3U8 parsing strategy.
- playlist recursive visited identity.
- BufferRequirement and PlatformCapabilityHints linkage.
- Golden Evidence Pack as offline artifact.
- three handoff ownership models only.
- RuntimeRevision priority/throttle/coalesce.
- optional field defaults.
- known_not_verified_items and accepted_deferrals in closure report.

## FC-007 Architecture Layer Gate

Goal: Lock dependency layers.

Evidence target:

- `backend/architecture/p3_architecture_policy.json`
- `artifacts/p3/evidence/p3_architecture_layer_check.json`

Allowed:

- Base layer: execution, security, evidence/contracts, source_model, platform_contracts, library_contracts.
- Middle layer: provider_integration, transports, sessions, media_delivery, media_probe.
- Decision layer: playback_decision, playback_handoff, runtime_revision.
- Test/tool layer: integration_harness, architecture_guard, golden_evidence, closure.

Forbidden:

- playback_decision depending on provider implementation or ffmpeg.
- provider depending on media_probe/ffmpeg or renderer/decoder/audio output.
- session depending on renderer/decoder/audio output.
- platform_contracts calling platform UI APIs.
- tools being runtime dependencies.

STOP if:

- dependency linter cannot machine-check the policy.

## FC-008 Evidence Dependency Gate

Goal: Prevent evidence modules from becoming runtime logic.

Evidence target:

- `artifacts/p3/evidence/p3_evidence_dependency_audit.json`

Allowed:

- evidence/contracts may be depended on by all modules.
- evidence/storage may be used by closure/test/evidence writer.
- Golden evidence may exist as test artifact.

Forbidden:

- evidence depending back on provider/probe/decision implementation.
- Golden evidence in normal playback success path.
- success path synchronous disk writes for golden snapshots.

STOP if:

- normal success path depends on golden evidence output.

## FC-009 P3 Directory Family Tree Gate

Goal: Create semantic family tree before files.

Evidence target:

- `docs/architecture/p3_natural_family_tree.md`
- architecture policy references to all P3 families.

Allowed:

- Use the document's family structure under `backend/src`, `backend/tests`, and `backend/tools`.
- Create subfamilies for execution, product_runtime, platform_contracts, library_contracts, source_model, provider_integration, providers, transports, sessions, media_delivery, media_probe, subtitle/audio/drm contracts, playback_decision, playback_handoff, security, evidence, integration_harness, architecture_guard.

Forbidden:

- flat files in root or broad source directories.
- provider_manager, source_manager.
- god_provider, universal_provider, one_big_provider.
- monolithic "p3.cpp" or "provider_all.cpp".

STOP if:

- implementation starts before family tree is written.

## FC-010 P3-M0 Contract Baseline Gate

Goal: Freeze base contracts and architecture guard.

Evidence target:

- `artifacts/p3/evidence/p3_m0_contract_baseline.json`
- `backend/architecture/p3_architecture_policy.json`

Allowed:

- Implement contract-only headers/types/tests for M0.
- Add architecture guard and forbidden boundary tests.

Forbidden:

- Starting fake providers or production hardening before core contracts compile and tests pass.
- Adding implementation hidden inside contract modules.

STOP if:

- any public contract lacks schema_version or optional field semantics.

## FC-011 P3-M1 Fake Provider Baseline Gate

Goal: Establish deterministic fake-provider integration without real accounts.

Evidence target:

- `artifacts/p3/evidence/p3_m1_fake_provider_baseline.json`
- fake provider harness evidence per provider.

Allowed:

- Fake HTTP range, WebDAV, Emby, Jellyfin, Alist, HLS, DASH, simple playlist.
- Real TCP loopback with random ports.
- test-controlled clock, barriers, cancellation.

Forbidden:

- real provider accounts.
- real private hosts.
- real tokens.
- sleep-race cancellation tests.

STOP if:

- fake provider cannot run deterministically.

## FC-012 P3-M1.5 Ownership And Security Gate

Goal: Verify critical ownership/security paths before production closure.

Evidence target:

- `artifacts/p3/evidence/p3_m15_ownership_security_baseline.json`

Allowed:

- Implement ownership, lease, cancellation, runtime revision, observer, DNS rebinding, CNAME, redirect loop, secure memory tests.

Forbidden:

- Expanding provider features while ownership/security critical path is incomplete.
- Closing P3 without runtime revision throttle/debounce.

STOP if:

- P4 lease ownership or observer lifecycle remains ambiguous.

## FC-013 P3-M2 Production Hardening Gate

Goal: Finish production-hardening closure.

Evidence target:

- `artifacts/p3/evidence/p3_m2_production_hardening.json`

Allowed:

- WebDAV/HTTP hardening, credential redaction, signed URL refresh semantics, probe process hardening, production contracts for sessions, cancellation, async, FastStart, preconnect, evidence retention/performance, P3->P4 handoff, candidate ranking, performance baseline, compatibility matrix, golden evidence, secret audit, forbidden scans, P2 regression, closure evidence/report.

Forbidden:

- Starting P4/P5/P6 playback implementation.
- Claiming complete production provider ecosystem.

STOP if:

- any M2 closure family lacks behavior test and evidence.

## FC-014 ExecutionContext Gate

Goal: Propagate context to all child work.

Evidence target:

- `artifacts/p3/evidence/execution_context_tests.json`

Allowed:

- Define operation_id, trace_context, cancellation_signal, deadline, budget_allocator, retry_budget, resource_budget, network_policy, evidence_policy, redaction_policy, task_group_policy, playback_latency_budget, platform_capability_hints, clock_provider, id_provider, feature_flags.

Forbidden:

- null context.
- implicit global context.
- mutable context reuse across user requests.
- platform hints triggering platform APIs.

STOP if:

- any provider/probe/session task can run without context.

## FC-015 MonotonicDeadline Gate

Goal: Use monotonic time for all timeout decisions.

Evidence target:

- `artifacts/p3/evidence/monotonic_deadline_tests.json`

Allowed:

- wall clock only for evidence display.
- context.clock_provider.now_monotonic as timeout authority.

Forbidden:

- wall clock as timeout authority.
- long-lived remaining_ms snapshots.

STOP if:

- any timeout path uses wall clock for control.

## FC-016 BudgetAllocator Gate

Goal: Define budget behavior for serial/parallel/cleanup work.

Evidence target:

- `artifacts/p3/evidence/budget_allocator_tests.json`

Allowed:

- shared_deadline, partitioned_budget, reserved_budget, best_effort with evidence.
- reserved cleanup/evidence/process-kill budget.

Forbidden:

- parallel tasks adding wall time incorrectly.
- best_effort as default supported path without evidence.

STOP if:

- cleanup or process kill can run without reserved budget.

## FC-017 RetryBudget And CircuitBreaker Gate

Goal: Bound retries and prevent storm behavior.

Evidence target:

- `artifacts/p3/evidence/retry_circuit_breaker_tests.json`

Allowed:

- retry only idempotent operations.
- Retry-After handling for 429/503.
- credential refresh single-flight.
- circuit breaker blocking new attempts only.

Forbidden:

- retry after cancellation.
- retry beyond deadline.
- interrupting P4 healthy session through circuit breaker.

STOP if:

- retry can spawn unbounded tasks.

## FC-018 Cancellation Propagation Gate

Goal: Every P3 long-running operation is cancellable.

Evidence target:

- `artifacts/p3/evidence/cancellation_propagation_tests.json`

Allowed:

- cancellation across provider request, HTTP, WebDAV, Emby/Jellyfin/Alist resolution, cloud link resolution, SourceSession read/seek, IndependentRangeReader, manifest fetch/refresh, playlist resolve, FastStart, MediaProbe process, decision task, evidence write, handoff.

Forbidden:

- detached uncancellable tasks.
- cancellation mapped to timeout.
- silent partial success after cancellation.

STOP if:

- cancellation tests depend on sleep race instead of barriers/latches/test clock.

## FC-019 Async Non-Blocking Gate

Goal: Prevent synchronous blocking in success paths.

Evidence target:

- `artifacts/p3/evidence/async_non_blocking_tests.json`

Allowed:

- owned async tasks.
- deadline-controlled long-running tasks.

Forbidden:

- network I/O blocking main thread.
- probe process wait blocking caller thread.
- detached task without owner.

STOP if:

- task ownership cannot be traced.

## FC-020 EventStream Backpressure Gate

Goal: Define event loss behavior.

Evidence target:

- `artifacts/p3/evidence/event_stream_backpressure_tests.json`

Allowed:

- progress events coalesce.
- critical events fail on overflow.
- evidence/session error events must not silently drop.

Forbidden:

- slow UI consumer blocking probe process kill.
- silent drop of SessionErrorSignal.

STOP if:

- critical event backpressure policy is missing.

## FC-021 RuntimeRevision Storm Control Gate

Goal: Throttle and prioritize runtime revisions.

Evidence target:

- `artifacts/p3/evidence/runtime_revision_storm_control_tests.json`

Allowed:

- max_revision_per_minute, min interval, coalesce window, priority order, duplicate dropping, deadline requirement.
- critical triggers outrank lower triggers.

Forbidden:

- UI hover or progress events as revision triggers.
- speculative quality switch without evidence.
- reopening sessions bypassing P4 ownership.

STOP if:

- rate limit evidence is not emitted when throttled.

## FC-022 FeatureFlag Closure Policy Gate

Goal: Ensure closure does not rely on experimental flags.

Evidence target:

- `artifacts/p3/evidence/feature_flag_closure_policy_tests.json`

Allowed:

- experimental flag paths with separate evidence.

Forbidden:

- experimental flag as only closure success path.
- default-on experimental closure path.

STOP if:

- closure fails with all experimental flags disabled.

## FC-023 Source / Provider / Transport / Delivery Model Gate

Goal: Keep origin, provider, transport, delivery, and container separate.

Evidence target:

- `artifacts/p3/evidence/source_model_layering_tests.json`

Allowed:

- SourceOriginKind describes item origin.
- ProviderFamily describes provider family.
- SourceTransportKind describes transport.
- MediaDeliveryKind describes delivery.
- ContainerFormat comes from probe/fast probe/provider metadata cross-validation.

Forbidden:

- HLS/DASH/HTTP/MP4 as ProviderFamily.
- URL suffix as final container evidence.
- provider_virtual entering P4 without resolving to concrete source/delivery or unsupported signal.

STOP if:

- one enum mixes provider, transport, delivery, and container.

## FC-024 ProviderBinding Gate

Goal: Bind provider item to source/session plan without becoming manager.

Evidence target:

- `artifacts/p3/evidence/provider_binding_tests.json`

Allowed:

- One-shot binding from SourceOriginIdentity + ProviderItemIdentity + optional credential to identity, capability hint, session/manifest/playlist plan, probe hints, fast start hints, decision hints, evidence.

Forbidden:

- provider_manager behavior.
- final PlaybackDecision generation.
- raw credential in evidence.

STOP if:

- ProviderBinding stores global provider registry state.

## FC-025 Provider Metadata Fast Path Gate

Goal: Let provider metadata help without replacing proof.

Evidence target:

- `artifacts/p3/evidence/provider_metadata_fast_path_tests.json`

Allowed:

- ProviderMetadataHint fields for container/codec/duration/stream/subtitle/HDR/DRM/confidence/source.
- ProbeSkipPolicy after P3 cross-validation.

Forbidden:

- provider self-claim alone skips probe.
- provider calls probe implementation.

STOP if:

- skip-probe lacks cross-validation for content type/length, magic, manifest parse, fast probe, and consistency.

## FC-026 WebDAV Production Hardening Gate

Goal: Upgrade P2 WebDAV not_verified items or honestly defer them.

Evidence target:

- `artifacts/p3/evidence/webdav_production_hardening_tests.json`

Allowed:

- href exact-match, collection rejection, malformed XML fail-closed, large XML body cap, PROPFIND depth policy, status taxonomy, redirect/auth/range/validator/path/encoding handling.

Forbidden:

- fake verified claims.
- ignoring the four P2 not_verified items.

STOP if:

- items remain unverified and are not in P3 closure known_not_verified_items with accepted deferral.

## FC-027 HTTP Production Transport Gate

Goal: Harden HTTP transport boundary.

Evidence target:

- `artifacts/p3/evidence/http_production_transport_boundary_tests.json`

Allowed:

- HEAD, GET range, 206, 200 fallback policy, 416, redirect, auth/cookie redaction, content length/type/etag/last modified evidence, short read/source changed, timeout/cancel taxonomy, retry-after, DNS/SSRF/TLS/proxy, connection pool, preconnect, identity range encoding, final connected IP validation.

Forbidden:

- compressed byte ranges claimed as original media seek.
- manifest gzip/br evidence omitted.
- production transport claim without tests.

STOP if:

- range read can accept non-identity Content-Encoding as seekable media bytes.

## FC-028 Emby Fake Foundation Gate

Goal: Model Emby without real account dependency.

Evidence target:

- `artifacts/p3/evidence/fake_emby_foundation_tests.json`

Allowed:

- fake Emby server with server/user/item/media source/playback info/direct stream/subtitle/audio/video/container/codec/transcode boundary/session lifecycle/credential refresh/redacted evidence.

Forbidden:

- real Emby account as closure blocker.
- "URL拼接" shortcut as foundation.
- full Emby client claim.

STOP if:

- fake server cannot prove source identity and candidate output.

## FC-029 Jellyfin Fake Foundation Gate

Goal: Model Jellyfin separately from Emby.

Evidence target:

- `artifacts/p3/evidence/fake_jellyfin_foundation_tests.json`

Allowed:

- reuse Emby-like contracts where appropriate with distinct provider family/evidence kind.

Forbidden:

- hardcoded Emby provider masquerading as Jellyfin.
- real account dependency.

STOP if:

- Jellyfin evidence cannot be distinguished from Emby evidence.

## FC-030 Alist Fake Foundation Gate

Goal: Model Alist file/direct-link flow.

Evidence target:

- `artifacts/p3/evidence/fake_alist_foundation_tests.json`

Allowed:

- fake Alist server identity, auth token, path identity, file entry, directory rejection, download link resolution, signed link expiry, required headers, range detection, redirect, credential redaction, link evidence.

Forbidden:

- real Alist account.
- raw token/link leak.

STOP if:

- signed link expiry and refresh semantics are not represented.

## FC-031 CloudDrive Contract Boundary Gate

Goal: Define cloud drive contract without SDK sprawl.

Evidence target:

- `artifacts/p3/evidence/cloud_drive_contract_boundary_tests.json`

Allowed:

- CloudDriveFamily, redacted account identity, file identity, link resolution, auth mode, quota/rate-limit/source evidence.
- ProviderUnsupportedCandidate and CloudDriveContractOnlySignal.

Forbidden:

- real cloud provider SDK implementation inside P3 closure.
- fake pass for unimplemented cloud provider.

STOP if:

- cloud boundary lacks unsupported signal.

## FC-032 SMB / UPnP-DLNA Boundary Gate

Goal: Define network media boundaries honestly.

Evidence target:

- `artifacts/p3/evidence/smb_upnp_dlna_boundary_tests.json`

Allowed:

- SmbShareIdentity, SmbFileIdentity, UpnpDlnaServerIdentity, UpnpDlnaItemIdentity, BoundaryUnsupportedReason.
- unsupported candidate when not implemented.

Forbidden:

- claiming SMB/DLNA playback support without implementation/evidence.

STOP if:

- boundary_defined_not_implemented is missing.

## FC-033 FTP / RTSP / RTMP Unsupported Gate

Goal: Produce explicit unsupported signals.

Evidence target:

- `artifacts/p3/evidence/unsupported_protocol_tests.json`

Allowed:

- UnsupportedProtocolCandidate, reason `protocol_out_of_p3_scope`, deferral stage P4_or_later.

Forbidden:

- fake pass.
- silent fallback to HTTP.

STOP if:

- unsupported protocol does not yield structured reason.

## FC-034 SourceSessionPlan Gate

Goal: Define source session plan inputs and policies.

Evidence target:

- `artifacts/p3/evidence/source_session_plan_tests.json`

Allowed:

- plan fields from identity to auth, signed URL, required headers, redacted headers, URL material ref, range/seek/content-encoding/connection/preconnect/proxy/TLS/expiry/refresh/cancellation/resource/reader policies, optional defaults.

Forbidden:

- raw URL in evidence.
- missing content_encoding_policy.
- tell() API.

STOP if:

- optional fields lack default or when-present semantics.

## FC-035 SourceSession Lifecycle Gate

Goal: Productionize SourceSession threading and lifecycle.

Evidence target:

- `artifacts/p3/evidence/source_session_lifecycle_tests.json`

Allowed:

- single-reader default.
- read/seek not concurrent.
- close/abort concurrent requests but idempotent.
- read result returns start/end offset.
- independent reader creation.
- observer registration.

Forbidden:

- tell().
- silent socket release while independent readers active.
- unowned observers.

STOP if:

- close/abort lifecycle is not deterministic.

## FC-036 IndependentRangeReader Gate

Goal: Establish independent range reader ownership.

Evidence target:

- `artifacts/p3/evidence/independent_range_reader_tests.json`

Allowed:

- independent reader without shared mutable seek position.
- strong session ref or lease-managed ownership.
- graceful close waits or deadline.
- abort invalidates readers immediately.
- P4 lease owner manages lifecycle after handoff.

Forbidden:

- raw pointer observer after handoff.
- callbacks after unregister.

STOP if:

- leak test cannot detect missing unregister.

## FC-037 ManifestSessionPlan Gate

Goal: Define manifest session plan and refresh policies.

Evidence target:

- `artifacts/p3/evidence/manifest_session_plan_tests.json`

Allowed:

- HLS/DASH manifest plan fields, fetch/refresh/staleness/snapshot/segment identity/content encoding/redirect/TLS/proxy/preconnect/cancellation/resource/refresh ownership/optional defaults.

Forbidden:

- segment queue playback.
- ABR execution.

STOP if:

- refresh ownership policy is missing.

## FC-038 ManifestSession Snapshot Gate

Goal: Ensure snapshot versions and candidate references are stable.

Evidence target:

- `artifacts/p3/evidence/manifest_snapshot_supersede_tests.json`

Allowed:

- new snapshot_version on refresh.
- candidates reference snapshot version.
- old snapshots governed by lease/supersede policy.

Forbidden:

- P4 using old snapshot as new manifest.
- immediate old snapshot destruction without policy.

STOP if:

- snapshot_supersede policy is not test-covered.

## FC-039 ManifestSessionLease Ownership Gate

Goal: Fix refresh ownership after handoff.

Evidence target:

- `artifacts/p3/evidence/manifest_session_lease_refresh_ownership_tests.json`

Allowed:

- refresh responsibility values: p3_refreshes_until_transfer, p4_refreshes_after_transfer, shared_refresh_via_runtime, reopen_by_plan_only.
- default snapshot policy keep_current_until_p4_ack.

Forbidden:

- P3 refreshing directly after P4 owns lease when policy says P4 owns refresh.
- P3 closing P4-held ManifestSession.

STOP if:

- ownership behavior depends on convention rather than contract.

## FC-040 Manifest Update Candidate Gate

Goal: Update candidates without controlling P4 playback.

Evidence target:

- `artifacts/p3/evidence/update_manifest_candidate_tests.json`

Allowed:

- produce new HLS/DASH candidate referencing new snapshot version.
- mark old candidate superseded/stale/kept_until_p4_ack.
- RuntimeRevisionResult suggests action.

Forbidden:

- P3 switching HLS variant or DASH representation.
- P3 controlling P4 segment selection.

STOP if:

- update_manifest_candidate alters P4 playback directly.

## FC-041 PlaylistSession Plan Gate

Goal: Define playlist session input, recursion, and auth policy.

Evidence target:

- `artifacts/p3/evidence/playlist_session_plan_tests.json`

Allowed:

- M3U, M3U8 simple, PLS, unknown.
- auth, signed URL, entry resolution, recursion, mixed scheme, relative path, cancellation, resource budget, optional defaults.

Forbidden:

- treating playlist as a continuous SourceSession byte stream.

STOP if:

- playlist plan cannot represent per-entry candidates.

## FC-042 Playlist Parsing Strategy Gate

Goal: Distinguish simple playlist from HLS.

Evidence target:

- `artifacts/p3/evidence/playlist_parsing_strategy_tests.json`

Allowed:

- #EXTM3U is necessary but not sufficient for HLS.
- HLS tags route to HLS parsing.
- simple URLs/EXTINF without HLS tags route to simple playlist.
- ambiguous policy default prefer_hls_if_hls_tags_present with evidence.

Forbidden:

- HLS misclassified as simple playlist silently.
- ambiguous playlist without evidence.

STOP if:

- ambiguous playlist behavior is not deterministic.

## FC-043 Playlist Recursion Gate

Goal: Prevent recursive and explosive playlists.

Evidence target:

- `artifacts/p3/evidence/playlist_recursion_policy_tests.json`

Allowed:

- max_depth, max_entries, allow_nested_playlist, loop prevention, mixed scheme policy.
- visited identity must be redacted stable identity.

Forbidden:

- raw URL in visited set.
- A -> B -> A loop.
- playlist bomb.
- credential leakage across entries.

STOP if:

- recursive loop does not emit playlist_entry_limit_exceeded or equivalent structured failure.

## FC-044 SessionErrorObserver Gate

Goal: Propagate errors across P3/P4 boundaries safely.

Evidence target:

- `artifacts/p3/evidence/session_error_observer_tests.json`

Allowed:

- on_session_error, state changed, closed events.
- session kind/source/kind/recoverable/retryable/revision/P4 action/evidence_ref.
- observer_id, weak token, auto unregister.

Forbidden:

- bare pointer observer ownership.
- silent critical overflow.
- callbacks after unregister.

STOP if:

- P4 handoff path cannot receive session errors.

## FC-045 Independent After Transfer Gate

Goal: Define responsibility after P4 takes lease.

Evidence target:

- `artifacts/p3/evidence/independent_after_transfer_tests.json`

Allowed:

- P4 close/abort ownership after lease.
- errors sent to P4 and RuntimeRevision if relevant.
- P3 emits recommendation, not direct closure.

Forbidden:

- P3 closing P4-held session.
- interpreting independent_after_transfer as no owner.

STOP if:

- P3 can silently invalidate P4 session without lease rule.

## FC-046 NetworkPolicy SSRF Gate

Goal: Enforce scheme/host/IP policies.

Evidence target:

- `artifacts/p3/evidence/network_policy_ssrf_tests.json`

Allowed:

- allowed/blocked scheme/host, private/loopback/link-local/multicast/file policies, redirects, CNAME depth, DNS attempts, cross-origin redirect, final connected IP validation, TLS/proxy.

Forbidden:

- default private IP access.
- auth forwarded cross-origin by default.

STOP if:

- SSRF tests do not include private IP ranges.

## FC-047 DNS Rebinding / CNAME Gate

Goal: Defend DNS rebinding through CNAME and final IP.

Evidence target:

- `artifacts/p3/evidence/dns_rebinding_cname_tests.json`

Allowed:

- traverse CNAME chain.
- fail max CNAME depth and CNAME loop.
- check forbidden final IPs.
- check IPv4, IPv6, and IPv4-mapped IPv6 private ranges.

Forbidden:

- checking only original hostname.
- trusting DNS public result without connected IP validation.

STOP if:

- connection pool reuse bypasses IP revalidation.

## FC-048 Redirect Security Gate

Goal: Revalidate every redirect hop.

Evidence target:

- `artifacts/p3/evidence/redirect_security_tests.json`

Allowed:

- re-check scheme, host, DNS, CNAME, final connected IP on every redirect.
- max_redirects, redirect loop detection.
- strip Authorization/Cookie cross-domain.

Forbidden:

- using initial host policy for final redirect target.
- preserving auth on cross-origin redirect by default.

STOP if:

- redirect target can bypass NetworkPolicy.

## FC-049 Final Connected IP Validation Gate

Goal: Validate actual connected IP at connection time.

Evidence target:

- `artifacts/p3/evidence/final_connected_ip_validation_tests.json`

Allowed:

- resolved_ip_set, selected_ip, connected_ip, checked flag, policy result evidence.
- connection pool revalidation on reuse and policy change.

Forbidden:

- resolving DNS once and trusting future connections.

STOP if:

- connected private IP from public DNS host is not blocked.

## FC-050 Credential Refresh Single-Flight Gate

Goal: Bound credential refresh concurrency.

Evidence target:

- `artifacts/p3/evidence/credential_single_flight_tests.json`

Allowed:

- one refresh per credential scope.
- concurrent requests join existing refresh.
- cancellation of one waiter does not cancel global refresh unless all waiters cancel.
- retry governed by RetryBudget.

Forbidden:

- refresh stampede.
- raw credential in logs/evidence.

STOP if:

- credential refresh can race and leak stale tokens.

## FC-051 SignedUrlHandle Gate

Goal: Model signed URLs without leaking them.

Evidence target:

- `artifacts/p3/evidence/signed_url_handle_tests.json`

Allowed:

- url_material_ref for real I/O.
- redacted_url for evidence.
- expiry/refresh ownership semantics.

Forbidden:

- passing redacted_url to HTTP client or ffprobe.
- raw signed URL in ProbeEvidence.

STOP if:

- signed URL raw material can enter committed files.

## FC-052 Secure Memory Gate

Goal: Make credential memory handling auditable.

Evidence target:

- `artifacts/p3/evidence/secure_memory_evidence_tests.json`

Allowed:

- secure memory states with reason and implementation detail.
- Windows as P3 primary platform.
- best_effort only with reason/audit evidence.

Forbidden:

- supported credential path with unsupported_blocking.
- signed_url_material below zeroized_after_use or stronger.
- token refresh path without zeroize evidence.

STOP if:

- secure_memory_state scan fails closure policy.

## FC-053 Redaction Policy Gate

Goal: Ensure sensitive data is sanitized everywhere.

Evidence target:

- `artifacts/p3/evidence/redaction_policy_tests.json`

Allowed:

- URL query, auth header, cookie, tokens, signed URL, password, username, private host, user path, provider account, session/operation IDs if user-derived, media file/title hints redaction.

Forbidden:

- raw filename-derived title by default.
- ffprobe raw stdout/stderr entering evidence.
- raw credential in child process env.

STOP if:

- secret audit finds raw credential/path/title leak.

## FC-054 Progressive Delivery Gate

Goal: Support progressive file candidates.

Evidence target:

- `artifacts/p3/evidence/progressive_delivery_tests.json`

Allowed:

- local file, HTTP/HTTPS range, WebDAV range, direct_url, provider resolved URL.
- output SourceSessionPlan, SourceSession, SourceCapability, FastStartEvidence, optional ProbeEvidence, DirectPlayCandidate.

Forbidden:

- claiming decoded media.

STOP if:

- progressive candidate lacks source capability evidence.

## FC-055 HLS Boundary Gate

Goal: Model HLS without implementing segment playback queue.

Evidence target:

- `artifacts/p3/evidence/hls_boundary_tests.json`

Allowed:

- HLS manifest identity/evidence, variants, segment identity, rendition, encryption, live/VOD, subtitles/audio renditions, live evidence, LL-HLS hints, HlsInputCandidate.

Forbidden:

- LL-HLS playback implementation.
- segment queue execution.

STOP if:

- HLS output contains segment scheduler behavior.

## FC-056 DASH Boundary Gate

Goal: Model DASH without DASH playback engine.

Evidence target:

- `artifacts/p3/evidence/dash_boundary_tests.json`

Allowed:

- MPD, period, adaptation set, representation, segment template/timeline, base URL, content protection, live/VOD, low-latency hints, DashInputCandidate.

Forbidden:

- DASH segment queue downloads.
- ABR execution.

STOP if:

- DASH module controls representation switching.

## FC-057 DRM Mixed Clear Gate

Goal: Handle mixed DRM/clear tracks correctly.

Evidence target:

- `artifacts/p3/evidence/drm_mixed_clear_tests.json`

Allowed:

- DRM metadata only.
- clear candidate priority.
- partial_drm_blocked at track level.

Forbidden:

- license request.
- decryption.
- marking whole manifest not playable when clear fallback is available.

STOP if:

- DRM decision lacks track-level evidence.

## FC-058 Media Probe Process Boundary Gate

Goal: Isolate ffprobe and probe logic.

Evidence target:

- `artifacts/p3/evidence/media_probe_process_boundary_tests.json`

Allowed:

- media_probe/contracts, process, ffmpeg families.
- local path, remote URL + safe headers, SourceSession byte reader inputs.
- Kivo-specific output contracts.

Forbidden:

- FFmpeg/ffprobe in providers, source_core, playback_decision.
- raw FFmpeg JSON exposed.
- media decode.

STOP if:

- provider directly invokes ffprobe implementation.

## FC-059 Fast Probe Gate

Goal: Provide minimal low-latency probe without lying.

Evidence target:

- `artifacts/p3/evidence/fast_probe_tests.json`

Allowed:

- MP4 moov, EBML, MPEG-TS PAT/PMT, FLV header, HTTP range minimal probe, HLS/DASH minimal parse, provider playback info, simple playlist entries.

Forbidden:

- full-file scan.
- full subtitle download.
- full chapter scan.
- artwork extraction.
- heavy sync evidence writes.

STOP if:

- FastProbe claims high confidence without CriticalConstraintsChecklist.

## FC-060 CriticalConstraintsChecklist Gate

Goal: Make confidence dependent on real constraints.

Evidence target:

- `artifacts/p3/evidence/critical_constraints_checklist_tests.json`

Allowed:

- source readable, range/manifest accessible, auth valid, known container/delivery, video/audio path, decoder/audio/subtitle/DRM/network/redaction/resource/handoff constraints.

Forbidden:

- confidence high when critical blocking false.
- ProbeEvidence downgrade without RuntimeRevision.

STOP if:

- confidence merge bypasses critical constraints.

## FC-061 ProbeEvidence Model Gate

Goal: Convert probe output to Kivo contracts.

Evidence target:

- `artifacts/p3/evidence/probe_evidence_model_tests.json`

Allowed:

- container, video, audio, subtitle, external subtitle, attachment, chapter, HDR, Dolby Vision, DRM, warnings, errors, confidence, critical constraints.

Forbidden:

- raw ffprobe JSON.
- raw paths or titles without redaction policy.

STOP if:

- ProbeEvidence lacks downgrade/update runtime trigger tests.

## FC-062 Subtitle Contract Gate

Goal: Support subtitle experience contracts without rendering.

Evidence target:

- `artifacts/p3/evidence/subtitle_contract_tests.json`

Allowed:

- download boundary, style, sync, burn-in decision contracts.

Forbidden:

- subtitle rendering.
- bitmap output.
- full external subtitle provider client as closure blocker.

STOP if:

- subtitle burn-in decision cannot express server processing required.

## FC-063 Audio Contract Gate

Goal: Support audio constraints without output device access.

Evidence target:

- `artifacts/p3/evidence/audio_contract_tests.json`

Allowed:

- audio track decision hints, passthrough candidate, playback speed audio hint.

Forbidden:

- opening audio device.
- outputting audio sample.

STOP if:

- passthrough candidate lacks platform hint and fallback.

## FC-064 Buffer Requirement Gate

Goal: Prepare P4 buffer requirements without executing buffer.

Evidence target:

- `artifacts/p3/evidence/buffer_requirement_tests.json`

Allowed:

- startup prebuffer, watermarks, max bytes, adaptation hint, seamless switch hint, independent reader policy.
- dynamic adjustment by power/data/network condition.

Forbidden:

- real buffer execution.
- prefetch/cache implementation.

STOP if:

- BufferRequirement cannot represent low-power/metered/cellular behavior.

## FC-065 PlaybackCandidateSet Gate

Goal: Build candidate set without playback.

Evidence target:

- `artifacts/p3/evidence/playback_candidate_set_tests.json`

Allowed:

- candidate types: direct play/stream, HLS/DASH/playlist, server processing required, not playable, probe required, provider unsupported, credential required, network unsupported, retryable network, unsupported protocol.

Forbidden:

- decoded/rendered/audio outputs.
- selected_by_p3 for ProbeRequiredCandidate unless requires_p4_selection.

STOP if:

- candidate set lacks not_claimed_fields.

## FC-066 CandidateRanking Gate

Goal: Rank candidates with policy reasons and no provider dependency.

Evidence target:

- `artifacts/p3/evidence/candidate_ranking_tests.json`

Allowed:

- playable first, fast start, lower risk, higher quality, less server processing.
- clear over DRM, direct play over transcode when platform supports, fresh manifest over stale.

Forbidden:

- provider implementation dependency.
- DRM prioritized over clear.
- server transcode prioritized over local DirectPlay when DirectPlay is valid.

STOP if:

- ranking result lacks reasons/rejected/deferred/pairwise comparisons.

## FC-067 PairwiseCandidateComparison Gate

Goal: Explain candidate choices pairwise.

Evidence target:

- `artifacts/p3/evidence/pairwise_candidate_comparison_tests.json`

Allowed:

- winner/loser, reason, winning/losing factors, policy rule.

Forbidden:

- opaque ranking with no comparison.

STOP if:

- key selected candidate lacks pairwise evidence.

## FC-068 PlaybackDecisionTrace Gate

Goal: Make decisions auditable and bidirectional with execution card.

Evidence target:

- `artifacts/p3/evidence/playback_decision_trace_tests.json`

Allowed:

- trace input constraints, evidence, missing evidence, ranking factors, pairwise comparisons, excluded candidates, runtime revision points, confidence, not_claimed_fields.

Forbidden:

- final trace without execution_card_id.
- trace calling provider/probe implementation.

STOP if:

- ExecutionCard and PlaybackDecisionTrace are not bidirectionally linked.

## FC-069 RuntimeRevision Gate

Goal: Support safe re-decision without controlling playback core.

Evidence target:

- `artifacts/p3/evidence/runtime_revision_tests.json`

Allowed:

- signed URL/provider session/probe/constraint/manifest/metadata/network/user preference/session error triggers.
- actions limited to keep/replace/downgrade/request processing/retry/not playable/refresh/rerank/update candidates/P4 action hint.

Forbidden:

- P3 direct HLS variant or DASH representation switch.
- P3 closing P4-held session.
- P3 controlling decoder/renderer.
- P3 downloading segment queue.

STOP if:

- runtime revision action crosses P4 ownership.

## FC-070 Handoff Ownership Gate

Goal: Constrain ownership to three models.

Evidence target:

- `artifacts/p3/evidence/handoff_ownership_model_tests.json`

Allowed:

- transfer_ownership, borrowed_until_close, reopen_by_plan.

Forbidden:

- shared_refcounted.
- ambiguous close responsibility.

STOP if:

- double close or dangling observer is possible in tests.

## FC-071 PlaybackHandoffContract Gate

Goal: Prepare P4 handoff without playing.

Evidence target:

- `artifacts/p3/evidence/playback_handoff_contract_tests.json`

Allowed:

- handoff fields: candidate set, optional selected candidate, session leases, ownership model, close responsibility, cancellation/deadline/session error bridge, hardware constraints, buffer requirement, runtime revision policy, evidence, state.

Forbidden:

- executing playback.
- controlling decoder/renderer/audio.

STOP if:

- handoff timeout does not reclaim P3-owned session.

## FC-072 FailureTaxonomy Gate

Goal: Normalize failure classes.

Evidence target:

- `artifacts/p3/evidence/failure_taxonomy_tests.json`

Allowed:

- binding, transport, probe, capability, handoff, runtime revision, manifest, playlist, session, security failures with detailed reasons.

Forbidden:

- generic internal_error for known failure classes.

STOP if:

- security or transport failures lack specific taxonomy.

## FC-073 Evidence Performance Gate

Goal: Keep evidence from blocking success path.

Evidence target:

- `artifacts/p3/evidence/evidence_performance_tests.json`

Allowed:

- success path sync evidence <= 5ms.
- success path disk write 0ms.
- failure path detailed evidence.
- debug raw evidence async only.

Forbidden:

- synchronous golden write on normal success.
- unbounded evidence growth.

STOP if:

- evidence success path exceeds budget without accepted degradation.

## FC-074 Golden Diff Policy Gate

Goal: Make golden evidence reproducible.

Evidence target:

- `backend/architecture/p3_golden_diff_policy.json`
- `artifacts/p3/evidence/golden_diff_policy_tests.json`

Allowed:

- ignore JSON object field order.
- path-specific array order policy.
- mask timestamps, ports, paths, operation ids.
- numeric tolerance for durations.

Forbidden:

- current commit hash inside committed closure evidence.
- raw dynamic fields that make golden unreproducible.

STOP if:

- golden diff cannot explain dynamic fields.

## FC-075 Schema Versioning Gate

Goal: Freeze public contract and evidence evolution.

Evidence target:

- `artifacts/p3/evidence/schema_versioning_tests.json`

Allowed:

- schema_version on all public contracts/evidence.
- optional/default or when-present semantics.
- migration policy.

Forbidden:

- breaking field drift after M2 closure.
- deleting fields without deprecation.

STOP if:

- any public contract lacks schema_version.

## FC-076 Platform Hints Gate

Goal: Keep platform facts as hints, not platform API implementation.

Evidence target:

- `artifacts/p3/evidence/platform_capability_hints_tests.json`

Allowed:

- Windows desktop primary.
- Qt as UI tech stack note only.
- D3D11/DXGI/WASAPI as hints/not_claimed.

Forbidden:

- real platform UI/media API calls.
- real audio/render device access.

STOP if:

- backend dependency links platform UI/render/audio implementation.

## FC-077 DRM Contract Gate

Goal: Identify DRM metadata without decrypting.

Evidence target:

- `artifacts/p3/evidence/drm_contract_tests.json`

Allowed:

- fairplay/widevine/playready/clearkey/HLS AES metadata, redacted KID/license/key URI hints, clear fallback.

Forbidden:

- license acquisition.
- decryption.
- decrypted sample output.

STOP if:

- DRM track blocks clear fallback incorrectly.

## FC-078 Observability Metrics Gate

Goal: Collect safe metrics only.

Evidence target:

- `artifacts/p3/evidence/observability_metrics_tests.json`

Allowed:

- operation/source/provider/transport/delivery/latency bucket/candidate/confidence/failure/retry/cancel/probe/manifest/playlist/handoff/revision kinds.

Forbidden:

- raw URL/path/username/token/cookie/signed URL/title/private host/display name.
- metrics blocking success path.

STOP if:

- metrics carry user media names or secrets.

## FC-079 Architecture Guard CI Gate

Goal: Make boundaries machine-checkable.

Evidence target:

- `backend/architecture/p3_architecture_policy.json`
- `artifacts/p3/evidence/architecture_guard_ci_tests.json`

Allowed:

- policy fields for modules, allowed/forbidden dependencies/includes/tokens/symbols, fixture allowlists, closure blocking.
- scanner differentiates code/comment/doc/test_fixture/policy/allowlisted occurrence.

Forbidden:

- runtime depending on architecture guard.
- unclassified forbidden token hits.

STOP if:

- architecture guard cannot run in CI/blocking mode.

## FC-080 Direct Executable Matrix Gate

Goal: Define direct executable evidence matrix.

Evidence target:

- `backend/architecture/p3_direct_executable_matrix.json`
- `artifacts/p3/evidence/direct_executable_matrix_result.json`

Allowed:

- matrix items with name, command, working directory, expected result, evidence output, closure blocking.

Forbidden:

- relying only on aggregate CTest logs for closure critical gates.

STOP if:

- any closure-blocking direct executable lacks evidence output.

## FC-081 Integration Harness Gate

Goal: Provide deterministic fake servers and fixtures.

Evidence target:

- `artifacts/p3/evidence/integration_harness_tests.json`

Allowed:

- in-process HTTP server, TCP loopback, random ports, StablePortMask, controlled clock/delay/cancellation.

Forbidden:

- real account closure blockers.
- sleep race cancellation.

STOP if:

- fake server cannot coordinate cancellation by barrier/latch/test clock.

## FC-082 Compatibility Matrix Gate

Goal: Document provider/delivery compatibility.

Evidence target:

- `artifacts/p3/evidence/compatibility_matrix_baseline.json`

Allowed:

- provider, min version, tested versions, known breaking changes, reverse proxy/CDN/auth/range/subtitle/HLS/DASH/manifest/playlist behavior, known limitations.

Forbidden:

- claiming untested vendor compatibility.

STOP if:

- known limitations are absent.

## FC-083 Soak Baseline Gate

Goal: Verify sustained input-chain behavior.

Evidence target:

- `artifacts/p3/evidence/soak_baseline_result.json`

Allowed:

- fake provider/fixture/controlled clock soak.
- 4-hour continuous input-chain or documented accelerated equivalent if project standard allows.
- frequent seek, revisions, signed URL refresh, manifest refresh, playlist resolution, track-switch signals, evidence growth, leaks, probe cancellation, session errors, reader invalidation, revision storm.

Forbidden:

- real account dependency.
- non-blocking nightly treated as closure evidence.

STOP if:

- handle/memory/evidence growth is unbounded.

## FC-084 Performance Baseline Gate

Goal: Prove input preparation latency and resource limits.

Evidence target:

- `artifacts/p3/evidence/performance_baseline_result.json`

Allowed:

- P99 targets for local/remote progressive, HLS, DASH, playlist, provider binding, fast probe, candidate ranking, handoff, evidence sync.
- graceful degradation above target but below hard ceiling with warning.

Forbidden:

- high-bitrate playback performance claim.
- unbounded memory/probe/zombie/evidence growth.

STOP if:

- hard ceiling is exceeded without timeout/retryable/probe_required output.

## FC-085 Final Capability Scenario Gate

Goal: Cover all final P3 scenarios.

Evidence target:

- `artifacts/p3/evidence/final_capability_scenarios.json`

Allowed:

- local file, HTTP Range, WebDAV, fake Emby/Jellyfin/Alist, CloudDrive contracts, HLS, DASH, simple playlist, unsupported FTP/RTSP/RTMP, hardware/audio/subtitle/DRM decisions, runtime revision, manifest/playlist updates, session error, independent reader invalidation, DNS/final IP, secure memory, performance, soak, golden evidence, P2 regression.

Forbidden:

- skipping scenario without known_not_verified_items or accepted_deferrals.

STOP if:

- P2 regression is not included.

## FC-086 Contract Acceptance Gate

Goal: Pass all contract acceptance families.

Evidence target:

- `artifacts/p3/evidence/contract_acceptance_result.json`

Allowed:

- source/provider/transport/delivery layering, latency, fast start, critical constraints, preconnect, platform/decoder/audio/subtitle/buffer/library/provider/session/manifest/playlist contracts.

Forbidden:

- closing P3 with contract-only tests missing.

STOP if:

- any public contract acceptance family fails.

## FC-087 Behavior Acceptance Gate

Goal: Pass all behavior acceptance families.

Evidence target:

- `artifacts/p3/evidence/behavior_acceptance_result.json`

Allowed:

- P2 regression, provider identity/binding/metadata/cross-validation, sessions/readers/observers/manifest/playlist/execution/budget/cancellation/async/event/retry/circuit/resource/credential/signed URL/handoff/runtime revision/ranking/comparison/trace tests.

Forbidden:

- skipping P2 regression.
- behavior tests hidden behind experimental flags only.

STOP if:

- any blocking behavior test fails.

## FC-088 Provider / Delivery Acceptance Gate

Goal: Pass all provider/delivery acceptance.

Evidence target:

- `artifacts/p3/evidence/provider_delivery_acceptance_result.json`

Allowed:

- WebDAV, HTTP content encoding, fake servers, CloudDrive/SMB/UPnP boundaries, unsupported protocols, HLS/LL-HLS/DASH/simple playlist.

Forbidden:

- vendor compatibility claims outside matrix.

STOP if:

- fake provider foundation test fails.

## FC-089 Probe / Media Evidence Acceptance Gate

Goal: Pass all probe/media evidence acceptance.

Evidence target:

- `artifacts/p3/evidence/probe_media_acceptance_result.json`

Allowed:

- media probe process, fast probe, minimal format probes, resource limits, VFR/audio language/disposition/passthrough/spatial/speed/external subtitle/burn-in/attachments/chapters/metadata triggers/HDR/Dolby/DRM/mixed clear tests.

Forbidden:

- decode/rendering.

STOP if:

- probe resource-limit tests fail.

## FC-090 Decision Acceptance Gate

Goal: Pass all decision acceptance.

Evidence target:

- `artifacts/p3/evidence/decision_acceptance_result.json`

Allowed:

- DirectPlay/DirectStream/HLS/DASH/playlist input decision tests, probe failure mapping, confidence merge, FastStart mapping, downgrade runtime revision, probe skip policy, ownership tests.

Forbidden:

- actual playback execution.
- decoder/renderer control.

STOP if:

- candidate decision lacks evidence trace.

## FC-091 Security Acceptance Gate

Goal: Pass all security acceptance.

Evidence target:

- `artifacts/p3/evidence/security_acceptance_result.json`

Allowed:

- NetworkPolicy/SSRF/CNAME/redirect/final IP/connection pool/DNS rebinding/credential/path/metadata redaction/secure memory/secret audit tests.

Forbidden:

- real secret in committed files.
- unsupported_blocking for supported credential path.

STOP if:

- secret audit has blocked findings.

## FC-092 Evidence / Closure Acceptance Gate

Goal: Pass evidence and closure tooling acceptance.

Evidence target:

- `artifacts/p3/evidence/evidence_closure_acceptance_result.json`

Allowed:

- evidence retention/performance, golden diff, golden pack, harness golden tests, compatibility, performance, graceful degradation, soak, architecture guard, target scan, static analysis, forbidden scan classification, no managers/god providers, no pollution, evidence/card/report consistency, coverage, blocking CI.

Forbidden:

- closure evidence mismatch with execution card.
- unclassified forbidden tokens.

STOP if:

- any blocking CI check fails.

## FC-093 Four-Layer Closure Gate

Goal: Close P3 only if contract, behavior, performance, and security gates pass.

Evidence target:

- `artifacts/p3/evidence/p3_four_layer_closure_gate.json`

Allowed:

- Contract Gate, Behavior Gate, Performance Gate, Security Gate as separate sections.

Forbidden:

- closing P3 if any layer fails.
- treating performance/security as advisory.

STOP if:

- any one of the four layers is not PASS.

## FC-094 Golden Evidence Pack Gate

Goal: Produce sanitized offline golden evidence pack.

Evidence target:

- `backend/tests/golden_evidence/p3/**`
- `artifacts/p3/evidence/golden_evidence_pack_result.json`

Allowed:

- fast_start, probe, manifest, playlist, playback_decision, decision_trace, pairwise_comparison, handoff, runtime_revision, session_error, redaction, cancellation, retry, closure.

Forbidden:

- raw token/cookie/signed URL/path/account/private host.
- dynamic unstable fields unmasked.

STOP if:

- structural diff cannot pass.

## FC-095 Closure Evidence Gate

Goal: Generate P3 closure evidence.

Evidence target:

- `artifacts/p3/evidence/P3-COMPLETE-LINE-V9-CLOSURE.json`

Allowed:

- schema `p3.closure.v9` or validator-supported equivalent.
- commit model policy without current HEAD hash in committed evidence.
- scope booleans for no UI/decode/render/audio/DRM license.
- required check matrix.
- closure gate results.
- redaction_state sanitized.

Forbidden:

- committed evidence tracking current commit hash.
- raw secrets.
- unsupported fields if validator rejects them.

STOP if:

- evidence schema validation fails.

## FC-096 Closure Report Gate

Goal: Generate P3 closure report.

Evidence target:

- `docs/roadmap/p3_complete_line_v9_closure_report.md` or project-approved closure report path.

Allowed:

- summary, scope boundary, completed lines, test matrix, architecture guard, coverage, performance, compatibility, soak, golden evidence, four gate results, evidence files, redaction, secret audit, forbidden scan, cancellation, async, budget, fast start, sessions, handoff, runtime revision, ranking, trace, probe, evidence retention/performance, known_not_verified_items, accepted_deferrals, external final report pointer.

Forbidden:

- "以后再说" deferrals.
- missing reason/risk/deferral_stage/blocking status.

STOP if:

- known_not_verified_items or accepted_deferrals are incomplete.

## FC-097 Risk Control Boundary Gate

Goal: Confirm each family stays in its boundary.

Evidence target:

- `artifacts/p3/evidence/p3_risk_control_boundary_audit.json`

Allowed:

- provider parses and produces hints.
- transport reads bytes.
- delivery parses manifests.
- sessions manage sessions.
- decision outputs candidates.
- handoff transfers leases.
- probe reads metadata under limits.

Forbidden:

- provider playing.
- transport decoding.
- delivery playing segment queue.
- manifest session switching variants.
- playlist session playing queue.
- decision playing.
- runtime decision controlling player.
- handoff executing playback.

STOP if:

- any boundary rule is violated.

## FC-098 P4 Handoff Boundary Gate

Goal: Define what P3 gives to P4 and what it does not.

Evidence target:

- `artifacts/p3/evidence/p4_handoff_boundary.json`

Allowed P3 -> P4:

- PlaybackCandidateSet, PlaybackHandoffContract, PlaybackDecisionRuntime, RuntimeRevisionResult.
- DirectPlayInputCandidate, DirectStreamInputCandidate, HlsInputCandidate, DashInputCandidate, PlaylistInputCandidate.
- FastStartEvidence, ProbeEvidence, ManifestEvidence, PlaylistEvidence.
- SourceSession, IndependentRangeReader, ManifestSession, PlaylistSession, SessionErrorObserver contracts.
- DecisionTrace, PairwiseComparison, cancellation bridge, retry signal, BufferRequirement, decoder/audio/subtitle constraints, DRM metadata, HDR delivery contract.

Forbidden P3 -> P4:

- decoded frame, render surface, audio device, subtitle bitmap, UI event, DRM decrypted sample, HLS/DASH segment scheduler, platform media session object, AirPlay route, Chromecast session.

STOP if:

- P4 handoff includes playback-core implementation objects.

## FC-099 Final Success Definition Gate

Goal: Verify P3 success definition is met.

Evidence target:

- `artifacts/p3/evidence/p3_final_success_definition.json`

Allowed:

- For supported provider item, produce execution context, budget, platform hints, identities, capability, plans, sessions/unsupported signal, reader if supported, observer, fast start/probe/manifest/playlist evidence, candidate set, ranking, pairwise, trace, handoff, runtime decision/revision, input candidates, buffer/constraints, DRM metadata, retry/error/cancellation/redacted evidence, execution card, failure reason, closure evidence.

Forbidden:

- success defined as "can open a link".

STOP if:

- final success lacks evidence traceability.

## FC-100 Final Wording Gate

Goal: Prevent broad product claims.

Evidence target:

- closure report final wording section.

Allowed wording:

- `P3 completes the playback-ready input system bridge.`
- `P3 prepares validated playback input candidates for P4.`
- `P3 is ready to hand off to P4 Playback Core Pipeline.`

Forbidden wording:

- `Kivo playback core complete.`
- `decoder/render/audio/UI complete.`
- `all providers production complete.`
- `HLS/DASH playback engine complete.`
- `DRM playback complete.`
- `Infuse/Kodi parity or surpass achieved.`

STOP if:

- final report contains broader product completion claims.

## FC-101 Final Report To User Gate

Goal: Make executor handoff understandable.

Evidence target:

- assistant final response after executing P3 work.

Allowed:

- Summarize completed gates, evidence paths, tests, known deferrals, and next stage.
- State whether P3 is closed or blocked.

Forbidden:

- claiming closure if any FC gate is incomplete.
- hiding accepted deferrals.

STOP if:

- final response does not mention blocked gates.
