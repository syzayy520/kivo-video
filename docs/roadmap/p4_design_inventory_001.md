# P4 Design Inventory Report

Task: KIVO-VIDEO-P4-DESIGN-INVENTORY-001
Type: design-inventory-only / no-code / no-implementation / no-commit-unless-report
Generated: 2026-06-27

## 0. Constraint

This report is a design material inventory only.
- NOT a P4 design document.
- NOT a P4 execution card.
- NOT P4 started.
- NOT Ready for P5.
- No code modified.
- No tests added.
- No CMake changed.

## 1. Files Read

| # | File | Purpose |
|---|------|---------|
| 1 | `docs/roadmap/p3_complete_line_v9_closure_ready_execution_card.md` | P3 execution card with FC-001..FC-101 gates |
| 2 | `docs/roadmap/p3_complete_line_v9_closure_report.md` | P3 final closure report with audit matrix |
| 3 | `artifacts/p3/evidence/FC-095.json` | P3 closure evidence with accepted deferrals |
| 4 | `docs/architecture/p3_natural_family_tree.md` | P3 directory family tree and dependency direction |
| 5 | `backend/architecture/p3_architecture_policy.json` | P3 four-layer architecture policy |
| 6 | `docs/roadmap/backend_task_checklist.md` | Backend task checklist with P2/P3 history |
| 7 | `docs/roadmap/current_state.md` | Current project state |
| 8 | `backend/include/kivo/p3/playback_handoff/handoff_contracts.hpp` | PlaybackHandoffContract struct fields |
| 9 | `backend/include/kivo/p3/playback_decision/decision_contracts.hpp` | RuntimeRevisionResult + PlaybackDecisionTrace |

## 2. P4 Design Slice Source Table

| Slice | Source File | Source Gate | Ready for P4 Design? |
|-------|------------|-------------|---------------------|
| PlaybackHandoffContract | handoff_contracts.hpp | FC-071, FC-098 | YES — 15 fields defined, ownership model locked |
| PlaybackDecisionRuntime | handoff_contracts.hpp | FC-071 | YES — runtime state placeholder |
| HandoffOwnershipModel | handoff_contracts.hpp | FC-070 | YES — 3 models locked (transfer/borrowed/reopen) |
| FailureTaxonomy | handoff_contracts.hpp | FC-072 | YES — 10 failure classes defined |
| PlaybackCandidateSet | decision_contracts.hpp | FC-065 | YES — 13 candidate kinds |
| CandidateRanking | decision_contracts.hpp | FC-066 | YES — policy-based, no provider dependency |
| PairwiseCandidateComparison | decision_contracts.hpp | FC-067 | YES — winner/loser/factors |
| PlaybackDecisionTrace | decision_contracts.hpp | FC-068 | YES — bidirectional with execution card |
| RuntimeRevisionResult | decision_contracts.hpp | FC-069 | YES — 9 triggers, 10 actions, P4 action hint |
| SourceSession | sessions.hpp | FC-035 | YES — lifecycle contract (single-reader, idempotent close) |
| IndependentRangeReader | sessions.hpp | FC-036 | YES — ownership after transfer (FC-045) |
| ManifestSession + snapshot supersede | sessions.hpp | FC-037..FC-040 | YES — refresh ownership 4 models |
| PlaylistSession | sessions.hpp | FC-041 | YES — recursion limits |
| SessionErrorObserver | sessions.hpp | FC-044 | YES — weak token, auto unregister |
| ExecutionContext | execution_context.hpp | FC-014 | YES — 17 fields, must propagate to P4 |
| CancellationSignal | cancellation_signal.hpp | FC-018 | YES — 4 states, must bridge to P4 |
| BufferRequirement | platform_contracts.hpp | FC-064 | YES — prebuffer/watermarks/max/dynamic |
| DecoderCapabilityRequirement | platform_contracts.hpp | FC-063 (partial) | PARTIAL — hints only, P4 must define actual decoder integration |
| AudioCapabilityRequirement | platform_contracts.hpp | FC-063 (partial) | PARTIAL — hints only, P4 must define audio output integration |
| SubtitleCapabilityRequirement | platform_contracts.hpp | FC-062 (partial) | PARTIAL — hints only, P4 must define subtitle rendering boundary |
| DrmMetadata | drm_contracts.hpp | FC-077 | YES — metadata only, no license/decryption |
| ProbeEvidence | probe_contracts.hpp | FC-061 | YES — container/codec/HDR/DRM/confidence/constraints |
| CriticalConstraintsChecklist | probe_contracts.hpp | FC-060 | YES — all critical blocking fields |
| FastStartEvidence | evidence_contracts.hpp | FC-073 (partial) | PARTIAL — contract defined, runtime measurement deferred |
| EvidencePerformancePolicy | evidence_contracts.hpp | FC-073 | YES — success path sync <=5ms, disk 0ms |
| NetworkPolicy | security_contracts.hpp | FC-046 | YES — P4 must honor during runtime |
| RuntimeRevisionStormControl | runtime_revision_storm_control.hpp | FC-021 | YES — throttle/coalesce/priority, P4 must consume |
| EventStreamBackpressure | event_stream_backpressure.hpp | FC-020 | YES — P4 must handle critical event overflow |

## 3. P3 -> P4 Handoff Input Checklist

P4 receives these from P3 at handoff (from FC-098):

### Allowed P3 -> P4 Objects

| Object | Contract Source | Key Fields |
|--------|----------------|------------|
| PlaybackCandidateSet | decision_contracts.hpp | set_id, candidates[], not_claimed_fields[] |
| PlaybackHandoffContract | handoff_contracts.hpp | handoff_id, candidate_set_ref, selected_candidate_ref, session_lease_ref, ownership, close_responsibility, cancellation_bridge, deadline_bridge, session_error_bridge, hardware_constraints_ref, buffer_requirement_ref, runtime_revision_policy, evidence_ref, state |
| PlaybackDecisionRuntime | handoff_contracts.hpp | runtime_id, handoff_ref, current_state, is_active |
| RuntimeRevisionResult | decision_contracts.hpp | revision_id, trigger, action, suggested_candidate_id, p4_action_hint |
| DirectPlayInputCandidate | (referenced in FC-098) | P4 must define struct |
| DirectStreamInputCandidate | (referenced in FC-098) | P4 must define struct |
| HlsInputCandidate | (referenced in FC-098) | P4 must define struct |
| DashInputCandidate | (referenced in FC-098) | P4 must define struct |
| PlaylistInputCandidate | (referenced in FC-098) | P4 must define struct |
| FastStartEvidence | evidence_contracts.hpp | evidence_id, fast_start_possible, confidence, constraints_ref, estimated_start_latency_ns |
| ProbeEvidence | probe_contracts.hpp | container, video_codec, audio_codec, subtitle_codec, hdr_format, drm_system, confidence, has_downgrade_trigger, constraints |
| ManifestEvidence | evidence_contracts.hpp | evidence_id, snapshot_version, is_stale |
| PlaylistEvidence | evidence_contracts.hpp | evidence_id, entry_count, recursion_policy_ref |
| SourceSession | sessions.hpp | session_id, plan_ref, is_open, current_offset, content_length, supports_independent_reader |
| IndependentRangeReader | sessions.hpp | reader_id, session_ref, is_active, has_strong_session_ref, lease_managed |
| ManifestSession | sessions.hpp | (via ManifestSessionPlan) snapshot_version, supersede_policy |
| PlaylistSession | sessions.hpp | (via PlaylistSessionPlan) playlist_kind, max_recursion_depth |
| SessionErrorObserver | sessions.hpp | observer_id, weak_token, auto_unregister, is_registered |
| PlaybackDecisionTrace | decision_contracts.hpp | trace_id, execution_card_id, input_constraints[], evidence_refs[], ranking_factors[], excluded_candidates[], runtime_revision_points[], confidence |
| PairwiseCandidateComparison | decision_contracts.hpp | comparison_id, winner_id, loser_id, reason, winning_factors[], losing_factors[], policy_rule |
| CancellationSignal | cancellation_signal.hpp | state (none/requested/propagating/completed), requested_at_ns |
| BufferRequirement | platform_contracts.hpp | startup_prebuffer_bytes, low_watermark_bytes, high_watermark_bytes, max_bytes, dynamic_adjustment_enabled |
| DecoderCapabilityRequirement | platform_contracts.hpp | codec_hint, hardware_decode_available, max_resolution, max_fps |
| AudioCapabilityRequirement | platform_contracts.hpp | audio_format_hint, passthrough_available, pcm_output_available, max_channels |
| SubtitleCapabilityRequirement | platform_contracts.hpp | bitmap_subtitle_supported, text_subtitle_supported, burn_in_supported |
| DrmMetadata | drm_contracts.hpp | system, redacted_kid, has_clear_fallback, partial_drm_blocked |

### Forbidden P3 -> P4 Objects (P4 must NOT receive these from P3)

- decoded frame
- render surface
- audio device handle
- subtitle bitmap
- UI event
- DRM decrypted sample
- HLS/DASH segment scheduler
- platform media session object
- AirPlay route
- Chromecast session

## 4. P4 Must-Accept Items

### 4.1 Accepted Deferrals from P3

| Gate | Name | Reason | P4 Must Do |
|------|------|--------|------------|
| FC-083 | Soak baseline | 4-hour continuous soak requires P4 integration environment | P4 must run real soak with PlaybackCore pipeline |
| FC-084 | Performance baseline | P99 measurement requires real runtime | P4 must measure P99 for local/remote/HLS/DASH/playlist |

### 4.2 P4 Core Responsibilities

| Responsibility | Source | Notes |
|---------------|--------|-------|
| PlaybackCore Pipeline | P3 forbidden outputs | P3 explicitly does NOT implement playback core |
| PlaybackSession lifecycle | P3 handoff contract | P4 owns session after transfer |
| State machine | P3 ExecutionContext | P4 must propagate context, manage playback states |
| Command queue | P3 ExecutionContext task_group_policy | P4 must implement command ordering |
| Read loop | P3 SourceSession + IndependentRangeReader | P4 drives read loop after handoff |
| Seek / pause / resume / stop / flush | P3 CancellationSignal | P4 must handle all playback controls |
| Timeline / position / buffered range | P3 BufferRequirement | P4 must track and report |
| Error recovery | P3 FailureTaxonomy (10 classes) | P4 must handle each failure class |
| Runtime revision handling | P3 RuntimeRevisionResult + StormControl | P4 must consume revisions, honor throttle |

## 5. P4 Forbidden Items

P4 must NOT do these (they belong to P5/P6/P7/P8 or later):

| Forbidden | Belongs To |
|-----------|-----------|
| Video decode implementation | P5 (Codec Pipeline) |
| Render / D3D11 / Vulkan | P6 (Render Pipeline) |
| HDR tone mapping | P6 (Render Pipeline) |
| WASAPI / audio device open | P7 (Audio Output) |
| Subtitle bitmap render | P6/P7 (Render/Subtitle) |
| UI / Qt pages | P8 (UI Layer) |
| Platform media session | P8 (Platform Integration) |
| Real P5/P6/P7/P8 work | Respective phases |

## 6. P4 and Subsequent Phase Boundaries

### P4 -> P5 Output (Codec Pipeline)

| Output | Notes |
|--------|-------|
| Compressed packets (from read loop) | P4 demuxes from SourceSession, passes to P5 |
| Stream selection | P4 selects audio/video/subtitle stream based on ProbeEvidence |
| Decoder capability requirements | P4 passes DecoderCapabilityRequirement to P5 |
| Seek target | P4 translates user seek to P5 seek target |

### P4 -> P6 Output (Render Pipeline)

| Output | Notes |
|--------|-------|
| Decoded frame request | P4 requests frames from P5, passes to P6 |
| Render timing | P4 provides presentation timestamp |
| HDR metadata | P4 passes HDR format from ProbeEvidence to P6 |
| Surface handle | P6 owns surface, P4 coordinates |

### P4 -> P7 Output (Audio Output)

| Output | Notes |
|--------|-------|
| Audio packet stream | P4 passes audio packets from P5 to P7 |
| Audio format | P4 passes AudioCapabilityRequirement to P7 |
| Passthrough decision | P4 decides passthrough vs PCM based on AudioCapabilityRequirement |
| Volume / mute | P4 controls, P7 executes |

### P4 -> P8 Output (UI Layer)

| Output | Notes |
|--------|-------|
| Playback state | P4 reports state to P8 |
| Position / duration | P4 reports timeline to P8 |
| Error messages | P4 reports FailureTaxonomy to P8 |
| Buffer status | P4 reports buffered range to P8 |

### P4 Must NOT Invade

- P5 codec selection logic (P4 selects stream, not codec)
- P6 render algorithm (P4 provides timing, not render method)
- P7 audio device management (P4 decides format, not device)
- P8 UI layout (P4 reports state, not UI structure)

## 7. P4 Design Risk Points

| Risk | Source | Impact |
|------|--------|--------|
| Handoff ownership ambiguity | 3 models (transfer/borrowed/reopen) | P4 must clearly define when each model applies |
| Runtime revision storm | StormControl defined but P4 must consume | P4 must implement revision consumer without blocking playback |
| Session error propagation | SessionErrorObserver weak token | P4 must ensure observer lifecycle doesn't dangle |
| Cancellation bridge | CancellationSignal 4 states | P4 must bridge P3 cancellation to P4 playback control |
| Buffer requirement dynamic adjustment | BufferRequirement has low_power/metered/cellular flags | P4 must react to platform condition changes |
| Manifest refresh ownership | 4 models (p3/p4/shared/reopen) | P4 must know which model is active per session |
| DRM clear fallback | DrmMetadata has_clear_fallback | P4 must check fallback before blocking playback |
| ProbeEvidence downgrade | has_downgrade_trigger flag | P4 must handle probe confidence downgrade during playback |
| IndependentRangeReader after transfer | IndependentAfterTransfer p4_owns_close/abort | P4 must own reader lifecycle after handoff |

## 8. P4 Design Gaps

| Gap | Description | Must Resolve Before P4 Design |
|-----|-------------|-------------------------------|
| InputCandidate structs | DirectPlayInputCandidate etc. referenced in FC-098 but not defined in P3 | P4 must define these structs |
| PlaybackSession state machine | P3 has PlaybackDecisionRuntime.current_state but no state enum | P4 must define playback state enum |
| Command queue specification | P3 has task_group_policy but no command types | P4 must define command types (seek/pause/resume/stop/flush) |
| Read loop specification | P3 has SourceSession but no read loop contract | P4 must define read loop contract |
| Timeline contract | P3 has BufferRequirement but no timeline/position contract | P4 must define timeline contract |
| Error recovery actions | P3 has FailureTaxonomy but no recovery action mapping | P4 must map each failure class to recovery action |
| Runtime revision consumer | P3 has RuntimeRevisionResult but no P4 consumer contract | P4 must define revision consumer contract |
| Soak test specification | FC-083 deferred but no P4 soak specification | P4 must define soak test specification |
| Performance target specification | FC-084 deferred but no P4 P99 target specification | P4 must define P99 targets per delivery kind |

## 9. Questions to Resolve Before Formal P4 Design Document

1. PlaybackSession state machine: what are the exact states and transitions?
2. Command queue: what command types exist? How are they ordered/superseded/coalesced?
3. Read loop: how does P4 drive SourceSession read loop? How does it handle backpressure?
4. Seek: how does P4 translate user seek to SourceSession seek + P5 codec flush?
5. Pause/resume: how does P4 pause read loop without losing session state?
6. Timeline: how does P4 calculate position from PTS? How does it handle VFR?
7. Error recovery: what is the recovery action for each of the 10 FailureClass values?
8. Runtime revision: how does P4 consume RuntimeRevisionResult without blocking playback?
9. Buffer management: how does P4 use BufferRequirement watermarks to control read loop?
10. Soak test: what is the exact 4-hour soak specification for P4?
11. Performance: what are the P99 targets for each delivery kind in P4?
12. Handoff ownership: when does each of the 3 ownership models apply in P4?
13. Manifest refresh: how does P4 know which of the 4 refresh ownership models is active?
14. DRM fallback: how does P4 switch from DRM track to clear fallback at runtime?
15. Probe downgrade: how does P4 handle ProbeEvidence downgrade during active playback?

## 10. Constraint Reaffirmation

- No code modified: YES (this report only)
- No tests added: YES
- No CMake changed: YES
- P4 started: NO
- Ready for P5: NO
- P4 design document written: NO (this is inventory only)
- P4 execution card written: NO
