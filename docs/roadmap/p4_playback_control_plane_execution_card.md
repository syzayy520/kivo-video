# P4 Playback Control Plane Execution Card

Updated: 2026-06-28

Canonical planning task id: `KIVO-VIDEO-P4-PLAYBACK-CONTROL-PLANE-DESIGN-LOCK-001-V8`.
Short task id: `P4-PLAYBACK-CONTROL-PLANE-V8`.
Source document: `Kivo Video P4 Playback Control Plane Design v1.8 Final Contract-Lock`.
Source DOCX: `C:\Users\Administrator\Downloads\Kivo Video P4 Playback Control Plane Design.docx`.

This execution card converts the P4 v1.8 design document into numbered gates for execution. It is intentionally explicit because P4 is the playback control plane — the most complex phase after P3.

## Core Position

P4 = Playback Control Plane.

P4 takes P3's selected playback input and converges it into a type-complete, revision-complete, zero-copy-memory-complete, queue-complete, reader-lifecycle-complete, shutdown-quarantine-complete, controllable, observable, interruptible, seekable, flushable, backpressure-capable, adaptive, deadlock-free, recoverable, safely-destructible, diagnosable, testable, long-term-evolvable playback control plane.

P4 does NOT do: decode, demux implementation, PTS/DTS parsing, render, audio output (WASAPI), subtitle rendering, provider API, provider selection, FFmpeg probe, codec inspection, frame scheduling, A/V sync correction, cache, prefetch, UI, HDR/Dolby/tone mapping, DRM decrypt, license acquisition.

## Dependency Order

P4 starts only after P3 closure is accepted:

1. `P3-COMPLETE-LINE-V9` (closed with accepted deferrals)
2. `P4-PLAYBACK-CONTROL-PLANE-V8`

P3 final baseline:
- Branch: `kivo-video-p3-preflight-gate-repair-001`
- HEAD: `791805e1378bbe675608c67cb01c40d9dd051ef9`
- Closure: CONTRACT_PASS_WITH_ACCEPTED_DEFERRALS
- Accepted deferrals to P4: FC-083 (soak), FC-084 (performance)

STOP if P3 closure is not accepted or P3 source/input bridge baseline is not locked.

## Global Rules

All gates must follow the Kivo iron gate:

1. Concept.
2. Planning design.
3. Natural family tree.
4. Implementation.
5. Single file, single responsibility.
6. No flat files.

P4 v1.8 additional global rules:
- All public interface types must be in Type Registry (`p4_type_registry.json`).
- P3/P4 type authority: P4 must NOT redefine P3 sealed types. Use Ref/View/Snapshot suffix.
- All units must be strong-typed (Milliseconds, Bytes, Count, BitsPerSecond, BytesPerSecond).
- RevisionStamp must carry SessionId/SessionGeneration/ReadEpoch/SeekEpoch/RecoveryEpoch/StreamRevision/TimelineRevision/RuntimePolicyRevision.
- Hot path must NOT use std::vector dynamic allocation; overflow must use pre-allocated arena.
- No callback under lock.
- No root/global manager bucket files.

## Global Allowed / Forbidden Scope

Allowed P4 outputs:

- PlaybackSession, PlaybackSessionHandle, PlaybackCommandRequest, PlaybackCommand, CommandQueue.
- SessionState, SessionState adjacency matrix, BufferingContext.
- RevisionStamp, RevisionBarrier, RevisionValidator, Cancellation.
- ReaderWorker, ReaderOrchestrator, ReaderInstanceState, ReaderReadResult, ReaderReadPayloadKind.
- MemoryLease, MutableMemoryBlock, ImmutableMemoryBlock, ReadOnlyMemoryChain, MemoryPool.
- BufferItem, BufferEngine, BufferSnapshot, BufferPushResult.
- PlaybackTimelineState, SourceTimelineState, LiveWindow, LiveStreamPolicy.
- BackpressureScheduler, AdaptiveBackpressure, TokenDebtPolicy.
- DownstreamFeedback, DownstreamAck, DownstreamFailure, ClockObservation, ClockObservationSlot.
- TrackSwitchFence, RequiredDownstreamAcks, AuthorizedTrackSwitchPlan.
- DiscontinuityMarker, RecoveryContext, RecoveryBudget, QuarantinedReaderRecord.
- EventBus, EventDeliveryClass, SessionEvent, EventDetail.
- SessionDiagnostics, StableReasonCode, StableErrorCode, FixedWindowHistogram.
- RuntimePolicyUpdate, RuntimePolicyPayload, RuntimePolicyKind.
- PlaybackOpenEnvelope, P3PlaybackHandoffRef, P3XXXRef/View types.
- VideoPipelineInputPlan, AudioPipelineInputPlan, SubtitlePipelineInputPlan (output boundaries).
- IMediaStreamPipeline, StreamPullResult, StreamPullStatus.
- ExecutionCard and ClosureEvidence.

Forbidden P4 outputs:

- decoded frame.
- render surface / D3D11 / DXGI / Vulkan.
- audio device open / WASAPI / audio sample output.
- subtitle bitmap / ASS/SSA rasterization / font shaping / OSD drawing.
- UI / Qt / QWidget / QML.
- demux implementation / PTS / DTS parsing.
- codec inspection / frame scheduling.
- A/V sync correction.
- cache system / prefetch system.
- HDR / Dolby / tone mapping.
- DRM decrypt / license acquisition / license server call.
- provider API / provider selection.
- FFmpeg probe.
- platform media session.

## Milestone Model

P4 must be executed as maturity layers:

- P4-M0: Contract + Type System + Family Tree + Architecture Guard.
- P4-M1: Behavior Tests (state machine, command queue, reader, memory, buffer, timeline, revision, recovery, eventbus, clock).
- P4-M2: Stress + Soak + Performance + Closure.

---

## P4G-001 P4 Boundary Gate

Goal: Lock P4 meaning before any implementation.

Evidence target:
- `docs/roadmap/p4_playback_control_plane_execution_card.md`
- future closure evidence field `p4_positioning_verified = true`

Allowed:
- State P4 as Playback Control Plane.
- Reference P3 handoff as input.
- Define P4 input/output boundaries.

Forbidden:
- Calling P4 "decode pipeline".
- Calling P4 "render pipeline".
- Claiming decode/render/audio/UI completion.

STOP if:
- Any task names P4 work as decode/render/audio/UI.

## P4G-002 P3 Closure Dependency Gate

Goal: Ensure P4 starts only after P3 closure.

Evidence target:
- `artifacts/p3/evidence/FC-095.json`
- `docs/roadmap/p3_complete_line_v9_closure_report.md`
- P4 closure report section `p3_closure_dependency`

Allowed:
- Read P3 closure evidence/card.
- Record P3 closure commit and locked baseline.

P3 final closure canonical source:
- Branch: `kivo-video-p3-preflight-gate-repair-001`
- HEAD: `791805e1378bbe675608c67cb01c40d9dd051ef9`
- Closure: CONTRACT_PASS_WITH_ACCEPTED_DEFERRALS
- Accepted deferrals to P4: FC-083 (soak), FC-084 (performance)

Forbidden:
- Starting P4 implementation if P3 closure is not accepted.
- Using P4 to repair unfinished P3 gates.

STOP if:
- P3 closure evidence/card is missing, invalid, or not accepted.

## P4G-003 P4 Natural Family Tree Gate

Goal: Create semantic family tree before files.

Evidence target:
- `docs/architecture/p4_natural_family_tree.md`

Allowed family structure (from v1.8 section 65):
- `backend/include/kivo/playback/control_plane/` with subfamilies: contracts/, primitives/, handoff/, session/, state/, control/, command/, revision/, reader/, worker/, buffer/, backpressure/, memory/, timeline/, clock/, live/, rate/, track/, discontinuity/, feedback/, recovery/, lifetime/, quarantine/, idle/, runtime/, diagnostics/, concurrency/, events/, output_boundary/, evidence/.
- `backend/tests/playback/control_plane/` with subfamilies: contracts/, primitives/, revision/, memory/, reader/, buffer/, queue/, feedback/, clock/, state/, timeline/, recovery/, events/, quarantine/, output_boundary/, compile_fail/, stress/.

Forbidden:
- Flat files in root or broad source directories.
- `common.hpp`, `helper.hpp`, `utils.hpp`, `manager.hpp`, `types.hpp`, `pipeline.hpp` as root bucket.
- `engine.hpp` as root bucket (but `*_engine.hpp` with business qualifier is allowed: `buffer_engine.hpp`, `timeline_engine.hpp`, etc.).
- `god_player.hpp`, `player_manager.hpp`, `playback_manager.hpp`, `pipeline_manager.hpp`, `misc.hpp`, `all_contracts.hpp`.

STOP if:
- Implementation starts before family tree is written.

## P4G-004 P4 Architecture Policy Gate

Goal: Lock dependency layers and forbidden dependencies.

Evidence target:
- `backend/architecture/p4_architecture_policy.json`
- `artifacts/p4/evidence/p4_architecture_layer_check.json`

Allowed layers:
- Base: primitives, contracts, handoff.
- Session: session, state, control, command.
- Revision: revision, cancellation.
- Reader: reader, worker, buffer, backpressure, memory.
- Timeline: timeline, clock, live, rate.
- Feedback: feedback, track, discontinuity.
- Recovery: recovery, lifetime, quarantine, idle.
- Runtime: runtime, diagnostics, concurrency, events.
- Output: output_boundary, evidence.

Forbidden dependencies:
- P4 depends on decode/render/audio_output/ffmpeg/UI.
- P4 redefines P3 sealed types.
- Tools are runtime dependencies.
- Playback decision depends on provider implementation.

STOP if:
- Dependency linter cannot machine-check the policy.

## P4G-005 Type Registry Gate

Goal: All P4 public types must be registered.

Evidence target:
- `backend/architecture/p4_type_registry.json`
- `artifacts/p4/evidence/p4_type_registry_check.json`

Allowed:
- Each type has: type_name, authority, p4_owns_definition, p4_may_read_fields, cross_thread_safe, header, hot_path_allowed, allocation_allowed, notes.
- P3 authority types only appear as Ref/View/Snapshot.

Forbidden:
- Unregistered type in public header.
- P3 authority type redefined in P4 namespace.

STOP if:
- Any public type lacks registry entry.

## P4G-006 P3/P4 Type Authority Gate

Goal: P4 must NOT redefine P3 sealed types.

Evidence target:
- `artifacts/p4/evidence/p4_p3_type_authority_check.json`

P3 sealed types (P4 must NOT redefine):
- PlaybackCandidateSet, SelectedPlaybackCandidate
- DirectPlayInputCandidate, DirectStreamInputCandidate, HlsInputCandidate, DashInputCandidate, PlaylistInputCandidate
- SourceSessionPlan, SourceSessionContract, SourceSessionLease
- IndependentRangeReaderContract, ManifestSessionContract, ManifestSessionLease, PlaylistSessionContract
- ProbeEvidence, CriticalConstraintsChecklist
- FailureTaxonomy, PlaybackDecisionTrace, PlaybackHandoffContract
- RuntimeRevisionResult, BufferRequirement, RedactionPolicy, NetworkPolicy, EvidenceRef

P4 must define instead:
- P3PlaybackHandoffRef, P3CandidateSetRef, P3SelectedCandidateRef
- P3SourceSessionRef, P3ReaderFactoryRef, P3ProbeEvidenceView, P3RuntimeRevisionView
- PlaybackOpenEnvelope, PlaybackInputBinding, PlaybackControlSessionConfig

Forbidden:
- Same type name in P4 namespace as P3 sealed type (unless Ref/View/ImportedSnapshot suffix).

STOP if:
- Duplicate type name found between P3 and P4 public headers.

## P4G-007 Duplicate Enum Gate

Goal: Each base enum has exactly one authority definition.

Evidence target:
- `artifacts/p4/evidence/p4_duplicate_enum_check.json`

Authority ownership (from v1.8 section 5):
- TimeMappingConfidence -> timeline/time_mapping_confidence.hpp
- StreamEndKind -> buffer/stream_end_kind.hpp
- SessionState -> state/session_state.hpp
- StreamType -> primitives/stream_type.hpp
- PlaybackFailureKind -> recovery/playback_failure_kind.hpp
- DiscontinuityReason -> discontinuity/discontinuity_reason.hpp
- DownstreamAckKind -> track/downstream_ack_kind.hpp
- MemoryPoolPressure -> memory/memory_pool_pressure.hpp
- BufferLevel -> buffer/buffer_level.hpp
- ReaderInstanceState -> reader/reader_instance_state.hpp
- ReaderReadPayloadKind -> reader/reader_read_payload_kind.hpp
- StreamPullStatus -> contracts/stream_pull_status.hpp
- EventDeliveryClass -> events/event_delivery_class.hpp
- RuntimePolicyKind -> runtime/runtime_policy_kind.hpp

Forbidden:
- Same enum class name in two P4 public headers.
- Same enum class name as P3 enum (unless Ref/View suffix).

STOP if:
- Duplicate enum class name found.

## P4G-008 Strong Unit Type Gate

Goal: All units must be strong-typed wrappers, not raw uint64_t aliases.

Evidence target:
- `backend/include/kivo/playback/control_plane/primitives/value_types.hpp`
- `artifacts/p4/evidence/p4_strong_unit_check.json`

Required types (from v1.8 section 7):
- Milliseconds, Microseconds, Nanoseconds, Bytes, BitsPerSecond, BytesPerSecond, Count.

Rules:
- explicit constructor.
- checked_add / checked_sub / saturating_add.
- Bytes must not pass to Milliseconds.
- BitsPerSecond and BytesPerSecond must explicit convert.
- bps -> Bps must divide by 8 with non-divisible diagnostic.

Forbidden:
- `using Milliseconds = std::uint64_t;` (raw alias).
- Implicit raw U64 conversion.

STOP if:
- Any unit type is a raw alias or allows implicit conversion.

## P4G-009 Ratio / PlaybackRateRatio Gate

Goal: Define Ratio and PlaybackRateRatio with gcd canonicalization.

Evidence target:
- `backend/include/kivo/playback/control_plane/primitives/ratio.hpp`
- `backend/include/kivo/playback/control_plane/rate/playback_rate_ratio.hpp`
- `artifacts/p4/evidence/p4_ratio_check.json`

Rules:
- denominator != 0.
- numerator != 0 (unless future 0x rate).
- gcd canonicalize ({2,2} -> {1,1}).
- Compare using checked 128-bit cross multiply.
- No implicit construction from double.
- 1.0x = {1,1}, 1.25x = {5,4}, 0.5x = {1,2}.

Forbidden:
- `double == 1.0` to check normal rate.
- Implicit double to Ratio.

STOP if:
- Ratio allows zero denominator or non-canonicalized values.

## P4G-010 ID / Epoch / Revision Master Model Gate

Goal: Define all ID and Epoch types with clear semantics.

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/revision_master_model.hpp`
- `artifacts/p4/evidence/p4_revision_master_model_check.json`

Required types (from v1.8 section 9):
- IDs: SessionId, CommandId, ReaderInstanceId, MemoryBlockId, MemoryPoolId, TrackSwitchFenceId, FeedbackId, EventSubscriptionId.
- Epochs: SessionGeneration, CommandSequence, ReadEpoch, SeekEpoch, RecoveryEpoch, StreamRevision, TimelineRevision, P4RuntimePolicyRevision.

Rules:
- SessionId identifies session.
- SessionGeneration identifies session lifecycle generation.
- ReadEpoch identifies reader/read generation.
- SeekEpoch identifies seek fence generation.
- RecoveryEpoch identifies recovery process generation.
- StreamRevision identifies P4 -> P5/P6/P7 encoded stream generation.
- TimelineRevision identifies timeline snapshot generation.
- P4RuntimePolicyRevision identifies P4 runtime policy update generation.
- CallbackToken provides nonce only, no identity semantics.

STOP if:
- Any epoch type is missing or has ambiguous semantics.

## P4G-011 RevisionStamp Gate

Goal: RevisionStamp must carry all 8 fields and enforce validation order.

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/revision_stamp.hpp`
- `artifacts/p4/evidence/p4_revision_stamp_check.json`

Required fields:
- session_id, generation, read_epoch, seek_epoch, recovery_epoch, stream_revision, timeline_revision, runtime_policy_revision, callback_token.

Validation order:
1. session_id
2. generation
3. read_epoch
4. seek_epoch
5. recovery_epoch
6. stream_revision
7. runtime_policy_revision
8. callback_token
9. reader instance validity

STOP if:
- RevisionStamp is missing any of the 8 required fields.

## P4G-012 SnapshotStamp Gate

Goal: SnapshotStamp for immutable cross-thread snapshot reads.

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/snapshot_stamp.hpp`

Required fields:
- session_id, generation, timeline_revision, stream_revision, seek_epoch, recovery_epoch, captured_at (MonotonicTimestamp).

Rules:
- publish uses release semantics.
- load uses acquire semantics.
- snapshot must be immutable.
- External threads do not directly read internal mutable state.

STOP if:
- SnapshotStamp allows mutation after publish.

## P4G-013 CallbackToken Gate

Goal: CallbackToken is nonce-only, no identity semantics.

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/callback_token.hpp`

Rules:
- nonce is monotonically increasing within same SessionId.
- token is not recycled or reused.
- token does not carry generation/read_epoch.
- token cannot independently determine identity.

STOP if:
- CallbackToken carries generation or epoch fields.

## P4G-014 Thread Model Gate

Goal: Define P4 logical threads and their boundaries.

Evidence target:
- `backend/include/kivo/playback/control_plane/concurrency/thread_model.hpp`
- `artifacts/p4/evidence/p4_thread_model_check.json`

Required threads:
- ControlThread, ReaderWorkerThread, P5PullThread, P6ClockObservationPath, EventDeliveryThread, DiagnosticsThread, TestHarnessThread.

Rules:
- ControlThread is sole writer of SessionState/RevisionBarrier/CommandQueue/TimelineState.
- ReaderWorkerThread may block on reader I/O but must NOT modify SessionState.
- P5PullThread calls IMediaStreamPipeline::pull as single logical consumer.
- P6ClockObservationPath must be realtime-safe for AudioOutput.
- EventDeliveryThread must NOT hold internal locks when calling user callbacks.
- DiagnosticsThread reads immutable snapshots only.
- Any cross-thread event must carry RevisionStamp or SnapshotStamp.

STOP if:
- Any thread violates its boundary (e.g., ReaderWorker modifies SessionState).

## P4G-015 RevisionBarrier Gate

Goal: Define RevisionBarrier with commit semantics and 3-layer validation.

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/revision_barrier.hpp`
- `artifacts/p4/evidence/p4_revision_barrier_check.json`

Required:
- RevisionBarrierKind enum: Seek, Stop, Recovery, RecoveryFallback, Flush, TrackSwitch, RuntimePolicyUpdate.
- RevisionBarrierResult struct: kind, new_stamp, commit, accepted.
- RevisionBarrierCommit struct: old_stamp, new_stamp, intent_accepted_at, committed_at, queue_watermark.
- RevisionBarrier class: begin_seek_barrier(), begin_stop_barrier(), begin_recovery_barrier(), begin_recovery_fallback_barrier(), begin_flush_barrier(scope), begin_track_switch_barrier(), begin_runtime_policy_update_barrier(), validate(stamp), current_stamp().

Rules:
- Barrier does NOT guarantee already-queued old events vanish immediately.
- Barrier guarantees: after new publish point, old stamps cannot be newly accepted.
- Already-queued old events must be rejected at consumer validation.

3-layer validation:
1. ReaderWorker callback entry: validate_fast(stamp) — fail: discard payload, record stale diagnostics, do not enqueue.
2. ControlEventQueue consumer: validate(stamp) — fail: do not enter SessionState/Timeline/Buffer, record stale event count.
3. BufferEngine push: validate(stamp) — fail: reject write, record stale push.

STOP if:
- RevisionBarrier lacks any of the 7 barrier kinds or 3-layer validation.

## P4G-016 RevisionValidator Gate

Goal: Lock-free validator for ReaderWorker (must NOT take SessionState lock).

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/revision_validator.hpp`
- `artifacts/p4/evidence/p4_revision_validator_check.json`

Required:
- RevisionValidator class: validate_fast(stamp), current_stamp_snapshot().

Rules:
- validate_fast must be lock-free or extremely lightweight atomic load.
- Must NOT wait for Control Thread.
- Must NOT take SessionState lock.
- Must NOT trigger callback.
- Must NOT allocate heap.

STOP if:
- RevisionValidator takes any lock or allocates heap.

## P4G-017 Cancellation Gate

Goal: Define CancellationToken, CancellationScope, ReaderCancelRequest.

Evidence target:
- `backend/include/kivo/playback/control_plane/revision/cancellation.hpp`

Required:
- CancellationToken: session_id, generation, read_epoch, nonce. Must NOT be externally constructible — only P4 cancellation factory.
- CancellationScope enum: CurrentReadOnly, ReaderInFlight, ReaderAndBuffer, StreamRevision, Session, Shutdown, DestructiveReaderAbort.
- ReaderCancelRequest: stamp, cancellation, scope.

Rules:
- Reader only accepts cancel with stamp matching current reader instance.
- Old token cannot cancel new read.
- Seek/Stop/Shutdown can wake infinite deadline read.
- DestructiveReaderAbort must enter diagnostics.

STOP if:
- CancellationToken can be externally constructed.

## P4G-018 Time / Deadline / Range Gate

Goal: Define MonotonicTimestamp, Deadline, MediaTime, SourceRange.

Evidence target:
- `backend/include/kivo/playback/control_plane/primitives/time_types.hpp`
- `backend/include/kivo/playback/control_plane/primitives/source_range.hpp`

Required:
- MonotonicTimestamp: ticks_ns.
- Deadline: at (MonotonicTimestamp), infinite (bool).
- ReadDeadline, PullDeadline: wrap Deadline.
- MediaTimeMs: I64 value (VOD absolute must be >= 0).
- MediaDeltaMs: I64 value (can be negative).
- SourceOffset: Bytes value.
- SourceRange: begin, end_exclusive (begin <= end_exclusive; begin == end_exclusive = empty).
- Unknown range uses std::optional<SourceRange>, NOT sentinel max.

Rules:
- infinite deadline only means no business timeout, does NOT mean cannot be woken by Stop/Seek/Cancel.
- P4 does NOT parse MediaTimeMs from PTS/DTS, only maintains control-plane timeline.

STOP if:
- SourceRange uses sentinel max for unknown, or MediaTimeMs allows negative VOD absolute.

## P4G-019 SeekTarget / SeekResolution Gate

Goal: Define seek target types and resolution.

Evidence target:
- `backend/include/kivo/playback/control_plane/control/seek_target.hpp`
- `backend/include/kivo/playback/control_plane/control/seek_resolution.hpp`

Required:
- SeekTargetType enum: TimelinePosition, SourceOffset, RelativeDelta.
- SeekAccuracy enum: FastApproximate, KeyframeHinted, ExactRequested, SourceNative.
- SeekTarget: type, absolute_position_ms (optional), relative_delta_ms (optional), source_offset (optional), accuracy.
- SeekResolutionKind enum: Exact, Approximate, KeyframeHinted, SourceOffsetOnly, Unsupported.
- SeekResolution: kind, resolved_position_ms (optional), resolved_source_offset (optional), requires_downstream_decode_preroll, control_plane_warmup_hint_ms.

Rules:
- TimelinePosition must provide absolute_position_ms.
- RelativeDelta must provide relative_delta_ms.
- SourceOffset must provide source_offset.
- VOD TimelinePosition must be >= 0.
- control_plane_warmup_hint_ms is read warmup suggestion, NOT codec/keyframe exact preroll promise.
- P4 does NOT parse keyframe, does NOT promise codec exact seek.

STOP if:
- SeekTarget validation rules are not enforced.

## P4G-020 P3 Handoff Import Gate

Goal: P4 imports P3 handoff via read-only views, not redefinition.

Evidence target:
- `backend/include/kivo/playback/control_plane/handoff/p3_handoff_ref.hpp`
- `backend/include/kivo/playback/control_plane/handoff/playback_open_envelope.hpp`
- `artifacts/p4/evidence/p4_p3_handoff_import_check.json`

Required:
- P3PlaybackHandoffRef: opaque_p3_handoff (const void*), p3_schema_version.
- PlaybackOpenEnvelope: session_id, source_session, reader_factory, candidate_set, selected_candidate, execution_context, buffer_requirement, interleave_profile, stream_layout_hint, read_throttle_policy, low_speed_policy, reader_stall_policy, shutdown_policy, pause_buffer_policy, playback_rate_policy, live_stream_policy, track_switch_policy, resource_budget, idle_timeout_policy, failure_policy, runtime_revision_hook, probe_evidence, recovery_plan, handoff_evidence.

Rules:
- P4 does NOT directly modify P3 handoff.
- P4 only reads via P3-approved adapter.
- P3/P4 field conflicts must be recorded as HandoffEvidenceConflict.
- selected_candidate is read-only, P4 must NOT change provider decision.

STOP if:
- PlaybackOpenEnvelope allows modification of selected_candidate or P3 handoff fields.

---

## P4G-021 ProbeEvidence / StreamLayoutHint / InterleaveProfile Gate

Goal: Define P3 probe evidence view and P4 layout/interleave profiles.

Evidence target:
- `backend/include/kivo/playback/control_plane/handoff/p3_probe_evidence_view.hpp`
- `backend/include/kivo/playback/control_plane/handoff/stream_layout_hint.hpp`
- `backend/include/kivo/playback/control_plane/handoff/interleave_profile.hpp`

Required:
- P3ProbeEvidenceView: has_duration, duration_ms, is_seekable, has_reliable_byte_to_time_mapping, is_live_like, estimated_bitrate_bps, source_range (optional), initial_time_range (optional), encrypted_hint.
- TimeMappingConfidence enum: None, Estimated, P3Hinted, P5Observed, Stable (authority: timeline/time_mapping_confidence.hpp).
- StreamLayoutHint: has_known_duration, has_known_bitrate, has_reliable_byte_to_time_mapping, is_live_like, is_seekable, has_multiple_tracks, may_have_sparse_keyframes, duration_ms, estimated_bitrate_bps, time_mapping_confidence, suggested_seek_warmup_ms, max_reasonable_warmup_ms, average_cluster_bytes, average_cluster_duration_ms.
- ContainerInterleaveKind enum: Unknown, LowInterleave, NormalInterleave, HeavyInterleave, BurstClustered, FragmentedMp4Like, BlurayLike, LiveLike.
- InterleaveProfile: kind, audio_video_interleaved, sparse_keyframe_risk, bursty_cluster_risk, unknown_time_mapping, recommended_readahead_ms, max_safe_readahead_ms, recommended_readahead_bytes, max_safe_readahead_bytes.

Conflict rule:
- StreamLayoutHint is P4 runtime layout policy authority.
- ProbeEvidence is raw evidence.
- If conflict: StreamLayoutHint wins, TimeMappingConfidence downgrades, Diagnostics records HandoffEvidenceConflict.

STOP if:
- TimeMappingConfidence is defined in more than one header.

## P4G-022 RuntimePolicyUpdate Gate

Goal: Define typed runtime policy update with one-kind-one-payload validation.

Evidence target:
- `backend/include/kivo/playback/control_plane/runtime/runtime_policy_update.hpp`
- `backend/include/kivo/playback/control_plane/runtime/runtime_policy_kind.hpp`
- `artifacts/p4/evidence/p4_runtime_policy_check.json`

Required:
- RuntimeRevisionHook: initial_revision, allow_runtime_policy_update, allow_resource_budget_update, allow_recovery_plan_update.
- RuntimePolicyKind enum: BufferRequirement, ReadThrottle, LowSpeed, ReaderStall, PauseBuffer, PlaybackRate, LiveStream, TrackSwitch, IdleTimeout, AdaptiveBackpressure, ResourceBudget, RecoveryPlan.
- RuntimePolicyPayload: kind + 12 optional policy fields (one per kind).
- RuntimePolicyUpdate: revision, kind, payload.

Rules:
- kind=BufferRequirement -> only buffer_requirement set.
- kind=ReadThrottle -> only read_throttle_policy set.
- Multiple payloads present simultaneously = FAIL.
- Payload missing = FAIL.
- Runtime update must enter Control Thread.
- Revision lower than current = reject, record stale update.
- Update must NOT change provider selection.
- Update must NOT refresh token/license.
- Update must NOT directly modify P3 evidence.

STOP if:
- RuntimePolicyPayload allows multiple simultaneous payloads or missing payload for kind.

## P4G-023 PlaybackSessionHandle Gate

Goal: Define external playback session handle and command submission.

Evidence target:
- `backend/include/kivo/playback/control_plane/session/playback_session_handle.hpp`
- `backend/include/kivo/playback/control_plane/command/playback_command.hpp`

Required:
- PlaybackCommandRequest: type, priority, seek_target (optional), flush_scope (optional), playback_rate (optional), user_track_switch_request (optional), cancellation_override.
- PlaybackCommand: id, sequence, generation, submitted_at, request.
- PlaybackSessionHandle: id(), submit_command_request(request), play(), pause(), seek(target), stop(), flush(scope), set_playback_rate(rate), request_user_track_switch(request), snapshot(), timeline_snapshot(), diagnostics_snapshot(), stream_pipeline(), subscribe(mask, callback), request_shutdown(policy).

Rules:
- CommandId/CommandSequence/submitted_at only generated by P4 submit path.
- P8 must NOT directly submit TrackSwitchPlan.
- P8 can only submit UserTrackSwitchRequest.
- TrackSwitchPlan must be authorized by P3/P5 before entering AuthorizedPlaybackControlPort.
- Callback calling submit_command_request must only enqueue, NOT synchronously execute.

STOP if:
- External code can construct PlaybackCommand with internal sequence/generation.

## P4G-024 AuthorizedPlaybackControlPort Gate

Goal: Define authorized control port for track switch and runtime policy.

Evidence target:
- `backend/include/kivo/playback/control_plane/control/authorized_playback_control_port.hpp`

Required:
- AuthorizedPlaybackControlPort: execute_track_switch(plan), apply_runtime_policy_update(update).

Rules:
- P8 does NOT directly hold AuthorizedPlaybackControlPort.
- Only P3/P5 authorized plans can enter this port.
- P4 does NOT reverse-call P3 for provider selection.
- Port calls must enter ControlThread serialization.

STOP if:
- AuthorizedPlaybackControlPort can be called without authorization.

## P4G-025 CommandQueue Gate

Goal: Define command queue with policy, submit status, and event ordering.

Evidence target:
- `backend/include/kivo/playback/control_plane/command/command_queue_policy.hpp`
- `backend/include/kivo/playback/control_plane/command/command_submit_status.hpp`
- `artifacts/p4/evidence/p4_command_queue_check.json`

Required:
- CommandQueuePolicy: normal_max_depth, critical_reserved_depth, max_queue_delay_ms, reject_when_full.
- CommandSubmitStatus enum: Accepted, AcceptedAlreadyInProgress, Coalesced, DuplicateIgnored, RejectedTerminated, RejectedInvalidState, RejectedQueueFull, RejectedResourceBudget, RejectedPolicy, RejectedInvalidContext.

Stop semantics:
- First Stop in non-Stopping/Terminated state returns Accepted.
- Stopping state: subsequent Stop returns AcceptedAlreadyInProgress.
- Terminated state: returns RejectedTerminated.
- Stop is NOT coalesced, NOT dropped.

Seek supersede event ordering:
- CommandAccepted(A) -> CommandAccepted(B) -> CommandSuperseded(A, superseded_by=B) -> CommandAccepted(C) -> CommandSuperseded(B, superseded_by=C) -> SeekStarted(C) -> SeekCommitted(C) -> CommandCompleted(C).
- Superseded command must NOT emit CommandCompleted.

STOP if:
- Stop is coalesced or dropped, or superseded command emits CommandCompleted.

## P4G-026 EmergencyControlLane Gate

Goal: Define emergency control lane for preemption.

Evidence target:
- `backend/include/kivo/playback/control_plane/control/emergency_control_lane.hpp`

Required:
- EmergencyControlLane: request_stop(), request_seek_fence(), request_shutdown(), stop_requested(), seek_fence_requested(), shutdown_requested(), wake_control_thread(), wake_reader_waiters().

Rules:
- Emergency Lane does NOT carry SeekEpoch.
- SeekEpoch only generated by Control Thread when consuming SeekCommand.
- Emergency Lane only does preemption, wake, atomic flag.
- Emergency Lane must NOT allocate heap.
- Emergency Lane must NOT call user callback.

STOP if:
- EmergencyControlLane allocates heap or calls user callback.

## P4G-027 SessionState Adjacency Matrix Gate

Goal: Define strict session state adjacency matrix.

Evidence target:
- `backend/include/kivo/playback/control_plane/state/session_state.hpp`
- `artifacts/p4/evidence/p4_session_state_adjacency_check.json`

Required:
- SessionState enum: Idle, Initializing, Ready, Playing, Paused, Seeking, Buffering, ErrorRecovery, Stopping, Terminated.

Strict adjacency list:
- Idle: -> Initializing, Terminated
- Initializing: -> Ready, Playing, Buffering, ErrorRecovery, Stopping, Terminated
- Ready: -> Playing, Seeking, Stopping, Terminated
- Playing: -> Paused, Seeking, Buffering, ErrorRecovery, Stopping, Terminated
- Paused: -> Playing, Seeking, Stopping, Terminated
- Seeking: -> Ready, Playing, Paused, Buffering, ErrorRecovery, Stopping, Terminated
- Buffering: -> Playing, Paused, Seeking, ErrorRecovery, Stopping, Terminated
- ErrorRecovery: -> Ready, Playing, Paused, Buffering, Stopping, Terminated
- Stopping: -> Terminated
- Terminated: -> none

Required:
- `bool is_valid_transition(SessionState from, SessionState to);`
- Tests must cover all from/to combinations (10x10 = 100 tests).

STOP if:
- Any invalid transition is allowed, or any valid transition is rejected.

## P4G-028 BufferingContext Gate

Goal: Define buffering context for pause-after-buffering override.

Evidence target:
- `backend/include/kivo/playback/control_plane/state/buffering_context.hpp`

Required:
- BufferingContext: state_before_buffering, state_command_sequence, user_pause_after_entered.
- StartBufferReason enum: InitialStart, SeekWarmup, RecoveryResume, PausedSeekWarmup, RateRelock.
- StartBufferDecision: satisfied, required_bytes, required_ms, confidence, reason_code (StableReasonCode).

Rules:
- Hysteresis does NOT hold pending state.
- StateMachine holds BufferingContext.
- Pause after Buffering must override pending Playing.
- Stop/Seek/Recovery clears BufferingContext.
- Hot path reason_code uses StableReasonCode, NOT std::string.

STOP if:
- BufferingContext is held by Hysteresis instead of StateMachine.

## P4G-029 ReaderTransport / ReaderCapability Gate

Goal: Define reader transport kind and capability.

Evidence target:
- `backend/include/kivo/playback/control_plane/reader/reader_transport_kind.hpp`
- `backend/include/kivo/playback/control_plane/reader/reader_capability.hpp`

Required:
- ReaderTransportKind enum: LocalSeekableFile, HttpRangeLike, RemoteObjectRange, RemoteStreaming, LanRemote, UnknownRemote.
- ReaderCapability: seekable, supports_fast_seek, supports_graceful_cancel, supports_destructive_cancel, cancel_interrupts_blocking_io, safe_to_reuse_after_cancel, safe_to_reuse_after_seek, supports_range_read, graceful_cancel_bound_ms (optional), destructive_cancel_bound_ms (optional), transport_kind.

Rules:
- Provider name must NOT enter P4 policy (only ReaderTransportKind).

STOP if:
- ReaderCapability contains provider name or provider-specific field.

## P4G-030 ReaderInstanceState Gate

Goal: Define reader instance state lifecycle.

Evidence target:
- `backend/include/kivo/playback/control_plane/reader/reader_instance_state.hpp`
- `artifacts/p4/evidence/p4_reader_instance_state_check.json`

Required:
- ReaderInstanceState enum: Registered, Opening, Open, Reading, Cancelling, Closing, Closed, Invalidated, Quarantined, Destroyed.

ReaderInstanceRegistry validation must check:
1. id exists
2. state is usable
3. stamp matches registered stamp range
4. not quarantined
5. not invalidated
6. not destroyed

STOP if:
- ReaderInstanceRegistry allows operation on quarantined/invalidated/destroyed reader.

## P4G-031 ReaderWorker Gate

Goal: Define reader worker interface and post status.

Evidence target:
- `backend/include/kivo/playback/control_plane/worker/reader_worker.hpp`
- `artifacts/p4/evidence/p4_reader_worker_check.json`

Required:
- ReaderWorkerPostStatus enum: Accepted, RejectedStopped, RejectedQueueFull, RejectedStaleStamp.
- ReaderWorker class: post_open(request), post_read(request), post_seek(request), post_close(request), request_stop_accepting_new_tasks(), cancel_current_task(scope), join(timeout).

Rules:
- ReaderWorker executes blocking Reader methods.
- ReaderWorker must NOT modify SessionState.
- ReaderWorker only submits ReaderEvent.
- ReaderWorker callback entry uses RevisionValidator::validate_fast.
- ReaderWorker queue full must return RejectedQueueFull.
- ReaderWorker must NOT directly call EventBus subscriber callback.

STOP if:
- ReaderWorker modifies SessionState or directly calls EventBus subscriber.

## P4G-032 ReaderOrchestrator Gate

Goal: Define reader orchestrator with clear mutation boundaries.

Evidence target:
- `backend/include/kivo/playback/control_plane/reader/reader_orchestrator.hpp`

Required:
- ReaderOrchestrator: open_initial_reader(envelope), schedule_read(priority), cancel_inflight(scope), seek_or_replace_reader(target), close_reader(force), replace_reader(fallback).

Boundaries:
- May mutate reader-local state and registry state.
- Must NOT mutate SessionState.
- Must NOT mutate TimelineState.
- Must NOT publish SessionEvent directly.
- All session-visible effects go through ControlEventQueue.

STOP if:
- ReaderOrchestrator mutates SessionState or TimelineState.

## P4G-033 ReaderReadResult Gate

Goal: Define ReaderReadResult with Data+EndMarker legal matrix.

Evidence target:
- `backend/include/kivo/playback/control_plane/reader/reader_read_result.hpp`
- `backend/include/kivo/playback/control_plane/reader/reader_read_payload_kind.hpp`
- `backend/include/kivo/playback/control_plane/buffer/stream_end_kind.hpp`
- `artifacts/p4/evidence/p4_reader_read_result_matrix_check.json`

Required:
- ReaderReadPayloadKind enum: DataOnly, DataAndEndMarker, EndMarkerOnly, WouldBlock, Cancelled, Timeout, LowSpeedViolation, Stalled, Failure.
- StreamEndKind enum: None, EndOfFile, LiveEdgeReached, Stopped, Cancelled.
- ReaderDataPayload: start_offset, bytes_read, memory (ReadOnlyMemoryChain), estimated_time_range (optional).
- StreamEndMarker: kind, stamp, final_offset (optional), projected_position_ms (optional), observed_at.
- ReaderReadResult: stamp, payload_kind, data (optional), end_marker (optional), failure (optional).

Legal matrix:
- DataOnly: data required, end_marker forbidden, failure forbidden.
- DataAndEndMarker: data required, end_marker required, failure forbidden.
- EndMarkerOnly: data forbidden, end_marker required, failure forbidden.
- WouldBlock: data forbidden, end_marker forbidden, failure forbidden.
- Cancelled: data forbidden, end_marker optional (kind=Cancelled), failure optional.
- Timeout: data forbidden, end_marker forbidden, failure required.
- LowSpeedViolation: data forbidden, end_marker forbidden, failure required.
- Stalled: data forbidden, end_marker forbidden, failure required.
- Failure: data forbidden, end_marker forbidden, failure required.

STOP if:
- ReaderReadResult allows illegal combination (e.g., DataOnly with end_marker).

## P4G-034 Memory Lease / Freeze / Publish Gate

Goal: Define zero-copy memory lease, freeze, publish contract.

Evidence target:
- `backend/include/kivo/playback/control_plane/memory/memory_lease.hpp`
- `backend/include/kivo/playback/control_plane/memory/mutable_memory_block.hpp`
- `backend/include/kivo/playback/control_plane/memory/immutable_memory_block.hpp`
- `artifacts/p4/evidence/p4_memory_lease_check.json`

Required:
- MemoryBlockState enum: FreeInPool, WritableLeased, FrozenPublished, BorrowedByConsumer, RetiredPendingRefs, Reclaimable.
- MutableMemoryBlock: id, pool_id, capacity, alignment, data (std::byte*).
- MutableBlockLease: block_id, pool_id, lease_nonce, block.
- ImmutableMemoryBlock: id, pool_id, capacity, size, alignment, data (const std::byte*), lifetime.
- MemoryLeaseStatus enum: Granted, BackpressureRequired, PoolExhausted, Cancelled.
- MemoryLeaseResult: status, lease (optional).
- IReadBufferAllocator: acquire(min_size, max_size, alignment), freeze_and_publish(lease, written_size), release_unpublished(lease).

Rules:
- freeze_and_publish must receive MutableBlockLease, NOT just MemoryBlockId.
- Prevents non-owner freeze, double freeze, stale block id freeze.
- ImmutableMemoryBlock does NOT own heavy deleter.
- data points to memory pool managed stable block.
- Lifetime holds lightweight token, no heavy reclaim in realtime/hot path.
- Actual reclaim by MemoryPool maintenance/control context.

STOP if:
- freeze_and_publish accepts MemoryBlockId instead of MutableBlockLease.

## P4G-035 ReadOnlyMemoryChain No-Heap Gate

Goal: Define ReadOnlyMemoryChain with inline segments, no hot-path heap allocation.

Evidence target:
- `backend/include/kivo/playback/control_plane/memory/read_only_memory_chain.hpp`
- `artifacts/p4/evidence/p4_read_only_memory_chain_check.json`

Required:
- ReadOnlyMemorySegment: block_id, block (ImmutableMemoryBlock), offset, size.
- SegmentList: segments (FixedArenaArray<ReadOnlyMemorySegment>).
- ReadOnlyMemoryChain: inline_segments (std::array<ReadOnlyMemorySegment, 4>), inline_count, overflow_segments (optional SegmentListRef), total_size.

Rules:
- Normal read uses inline_segments only (up to 4).
- Heap overflow forbidden in hot path.
- Overflow only uses pre-allocated arena/pool.
- offset + size must not overflow.
- offset + size <= block.size.
- block.size <= block.capacity.

STOP if:
- ReadOnlyMemoryChain uses std::vector in hot path or allocates heap for normal reads.

## P4G-036 MemoryPool Ownership Gate

Goal: Define memory pool policy and reuse rules.

Evidence target:
- `backend/include/kivo/playback/control_plane/memory/memory_pool_policy.hpp`
- `artifacts/p4/evidence/p4_memory_pool_check.json`

Required:
- MemoryPoolStrategy enum: FixedSlab, SizeClassSlab, LargeBlockFallback.
- MemoryPoolPolicy: strategy, default_block_size, large_block_threshold, alignment_bytes, max_total_bytes, max_block_count, allow_reuse, allow_backpressure_when_exhausted, shrink_on_pause, reclaim_on_seek, reclaim_on_stop, reclaim_on_idle.

Reuse rules:
- Only state == FreeInPool or Reclaimable can be reused.
- FrozenPublished / BorrowedByConsumer / RetiredPendingRefs must NOT be reused.
- refcount is safety guard, not business state.

Deleter rules:
- shared_ptr/lifetime token deleter must NOT do heavy MemoryPool work.
- Deleter may enqueue lightweight retired-block token.
- Actual reclaim on MemoryPool maintenance/control context.
- No heavy reclaim under internal lock.

STOP if:
- MemoryPool reuses FrozenPublished or BorrowedByConsumer blocks.

## P4G-037 BufferItem Ordered Stream Gate

Goal: Define ordered buffer item stream with data/discontinuity/end marker.

Evidence target:
- `backend/include/kivo/playback/control_plane/buffer/buffer_item.hpp`
- `artifacts/p4/evidence/p4_buffer_item_check.json`

Required:
- BufferItemKind enum: Data, Discontinuity, EndMarker.
- BufferItem: kind, stamp, chunk (optional), discontinuity (optional), end_marker (optional).

Rules:
- BufferEngine maintains one ordered item stream.
- Data, discontinuity, EOF, LiveEdge all preserve order.
- Marker occupies logical queue position.
- Marker may be flushed by revision barrier.
- Marker is NOT silently dropped.
- ReaderReadResult::DataAndEndMarker must be split into BufferItem(Data) + BufferItem(EndMarker).

STOP if:
- BufferItem allows silent marker drop.

## P4G-038 LiveEdge / EOF Propagation Gate

Goal: Separate LiveEdge from EOF semantics.

Evidence target:
- `backend/include/kivo/playback/control_plane/buffer/stream_end_marker.hpp`
- `artifacts/p4/evidence/p4_live_edge_eof_check.json`

Rules:
- Reader returns ReaderReadResult::DataAndEndMarker(end_kind=LiveEdgeReached).
- BufferEngine enqueues: BufferItem(Data) + BufferItem(EndMarker: LiveEdgeReached).
- P5 pull sequence: pull Data until drained, then pull LiveEdgeReached.
- LiveEdgeReached is never Recovery.
- Internal: LiveEdgeReached is not EOF.
- Policy: if treat_live_edge_as_eof=true, SessionEvent may publish EndReached(live_edge_projected_as_eof), but StreamEndKind remains LiveEdgeReached.
- EndProjectionKind enum: TrueEof, LiveEdgeProjectedAsEof.

STOP if:
- LiveEdgeReached is treated as EOF internally, or StreamEndKind is changed by projection.

## P4G-039 StreamPullResult Matrix Gate

Goal: Define StreamPullResult with legal combination matrix.

Evidence target:
- `backend/include/kivo/playback/control_plane/contracts/stream_pull_result.hpp`
- `backend/include/kivo/playback/control_plane/contracts/stream_pull_status.hpp`
- `artifacts/p4/evidence/p4_stream_pull_matrix_check.json`

Required:
- StreamPullStatus enum: Data, WouldBlock, EndOfStream, LiveEdgeReached, Discontinuity, Fenced, Flushed, Cancelled, Terminated, Error.
- StreamPullResult: status, chunk (optional), wait_hint (optional), discontinuity (optional), end_marker (optional), stream_revision, timeline_revision, failure (optional).

Legal matrix:
- Data: chunk required, wait_hint optional, others forbidden.
- WouldBlock: wait_hint required, others forbidden.
- EndOfStream: end_marker required (kind=EndOfFile), others forbidden.
- LiveEdgeReached: end_marker required (kind=LiveEdgeReached), wait_hint optional, others forbidden.
- Discontinuity: discontinuity required, wait_hint optional, others forbidden.
- Fenced: wait_hint/discontinuity optional, others forbidden.
- Flushed: wait_hint/discontinuity optional, others forbidden.
- Cancelled: end_marker optional (kind=Cancelled), failure optional, others forbidden.
- Terminated: end_marker optional (kind=Stopped), failure optional, others forbidden.
- Error: failure required, wait_hint optional, others forbidden.

Rules:
- MediaSourceChunk no longer contains duplicated discontinuity/end_marker.
- Ordered marker lives in BufferItem.
- Pull result exposes current item according to matrix.
- Data result never carries end_marker.

STOP if:
- StreamPullResult allows illegal combination.

## P4G-040 IMediaStreamPipeline Gate

Goal: Define single logical consumer pipeline interface.

Evidence target:
- `backend/include/kivo/playback/control_plane/output_boundary/i_media_stream_pipeline.hpp`

Required:
- IMediaStreamPipeline: pull(request), snapshot(), submit_feedback(request), submit_clock_observation(observation), subscribe(mask, callback).

Rules:
- IMediaStreamPipeline is single logical consumer.
- Recommended: only P5 demux worker calls pull().
- If multiple threads call pull(), Pipeline must serialize into one ordered stream.
- Multiple independent consumers directly pulling encoded bytes are forbidden.
- P4 does NOT demux pulled data.

STOP if:
- Multiple independent consumers can pull encoded bytes simultaneously.

## P4G-041 DownstreamFeedback Channel Gate

Goal: Define downstream feedback queue with critical lane.

Evidence target:
- `backend/include/kivo/playback/control_plane/feedback/downstream_feedback_queue.hpp`
- `artifacts/p4/evidence/p4_feedback_queue_check.json`

Required:
- DownstreamDemandHintRequest: stream_revision, decoder_waiting_for_data, downstream_starved, downstream_waiting_for_discontinuity_data, after_seek_warmup, preferred_next_bytes, preferred_next_time_ms, stream_type, submitted_at.
- DownstreamDemandHint: id (FeedbackId), request.
- DownstreamAck: stream_revision, timeline_revision, generation, fence_id (optional), track_switch_plan_id (optional), kind (DownstreamAckKind), submitted_at.
- DownstreamFailure: generation, timeline_revision, stream_revision, stream_type, failure (PlaybackFailure).
- DownstreamFeedbackQueuePolicy: normal_max_depth, critical_reserved_depth, coalesce_demand_hint.

Rules:
- DemandHint: coalesce latest per stream revision.
- ClockObservation: goes to ClockObservationSlot, NOT queue. Return AcceptedToClockSlot.
- DownstreamAck: critical lane, NOT droppable.
- DownstreamFailure: critical lane, NOT droppable.
- Critical overflow: set critical_overflow flag, wake Control Thread.

STOP if:
- DownstreamAck or DownstreamFailure can be dropped on queue overflow.

## P4G-042 ClockObservation Realtime-Safe Gate

Goal: Define ClockObservationSlot with realtime-safe publish for AudioOutput.

Evidence target:
- `backend/include/kivo/playback/control_plane/clock/clock_observation_slot.hpp`
- `artifacts/p4/evidence/p4_clock_realtime_check.json`

Required:
- ClockObservation: session_id, generation, stream_revision, based_on_revision, source (ClockSource), observed_position_ms, observed_at, playback_rate, drift_ms (optional).
- ClockObservationSlot: publish_latest_realtime_safe(observation), publish_latest_non_realtime(observation), load_latest(source), snapshot().

Implementation contract for ClockSource::AudioOutput:
- Must use publish_latest_realtime_safe.
- Wait-free or bounded lock-free.
- No mutex.
- No heap allocation.
- No waiting for Control Thread.
- No EventBus publish.
- No callback.
- Recommended: sequence counter / double-buffer atomic slot. Writer stores payload then increments sequence release. Reader loops bounded times with acquire.

Stale rules:
- generation mismatch: drop.
- stream_revision mismatch: drop or record stale observation.
- based_on_revision older than current: drop or record stale.
- based_on_revision newer than current: reject invalid.

STOP if:
- ClockObservationSlot uses mutex, heap allocation, or blocks on Control Thread for AudioOutput path.

## P4G-043 TrackSwitch Required Ack Gate

Goal: Define TrackSwitchFence with required ack set.

Evidence target:
- `backend/include/kivo/playback/control_plane/track/required_downstream_acks.hpp`
- `backend/include/kivo/playback/control_plane/track/track_switch_fence.hpp`
- `artifacts/p4/evidence/p4_track_switch_ack_check.json`

Required:
- RequiredDownstreamAcks: kinds (std::array<DownstreamAckKind, 8>), count.
- FallbackAuthorizationKind enum: SameCredential, RefreshedCredential, DifferentCandidate, LicenseRefreshed, ProviderAuthorized.
- AuthorizedTrackSwitchPlan: plan_id, authorized_revision, authorization_token, stream_type, required_acks, stable_reason_code.
- TrackSwitchPolicy: allow_midstream_track_switch, requires_p3_authorization, requires_stream_revision_increment, requires_downstream_ack, fence_ack_timeout_ms.
- TrackSwitchFence: arm(id, new_revision, required_acks, armed_at), submit_ack(ack), all_required_ack_received(), expired(now, policy), release(), fail().

Rules:
- Timeout authority is TrackSwitchPolicy.fence_ack_timeout_ms.
- Required ack set comes from AuthorizedTrackSwitchPlan.
- all_required_ack_received compares ack kind + fence id + stream revision.
- Partial ack timeout must emit TrackSwitchFenceTimeout.

STOP if:
- TrackSwitchFence lacks required ack set, or all_required_ack_received cannot be implemented.

## P4G-044 Discontinuity Gate

Goal: Define discontinuity marker with reason/revision/target/fence.

Evidence target:
- `backend/include/kivo/playback/control_plane/discontinuity/discontinuity_reason.hpp`
- `backend/include/kivo/playback/control_plane/discontinuity/discontinuity_marker.hpp`

Required:
- DiscontinuityReason enum: SeekSnapshot, Flush, Stop, RecoveryReopen, NetworkSplice, AuthorizedFallbackSwitch, TrackSwitch, SourceRevisionChanged, RateChanged, LiveWindowJump, Unknown.
- DiscontinuityMarker: reason, generation, seek_epoch, recovery_epoch, old_revision, new_revision, target_position_ms (optional), target_source_offset (optional), requires_downstream_ack, fence_id (optional), emitted_at.

Rules:
- P4 does NOT tell P5/P6/P7 how to flush.
- P4 only emits reason/revision/target/fence.
- P5/P6/P7 decide their own flush/reset/relock behavior.

STOP if:
- DiscontinuityMarker instructs P5/P6/P7 how to flush.

## P4G-045 BackpressureScheduler Gate

Goal: Define backpressure scheduler with priority order and token debt.

Evidence target:
- `backend/include/kivo/playback/control_plane/backpressure/backpressure_scheduler.hpp`

Required:
- ReadSchedulingDecision: should_schedule_read, requested_bytes, priority, reason_code (StableReasonCode).
- BackpressureScheduler: decide(buffer, resource_budget, pressure, latest_hint, adaptive).
- TokenDebtPolicy: allow_urgent_token_debt, max_urgent_token_debt.

Priority order:
Stop/Seek/Shutdown > MemoryPoolCritical > ResourceBudget > TokenBucket > FastDrainUrgent > AdaptiveCooldown

STOP if:
- BackpressureScheduler lacks priority order or allows urgent read to break ResourceBudget/MemoryPoolCritical.

## P4G-046 AdaptiveBackpressure Gate

Goal: Define adaptive backpressure with grow/shrink thresholds and guard.

Evidence target:
- `backend/include/kivo/playback/control_plane/backpressure/adaptive_backpressure.hpp`

Required:
- AdaptiveBackpressureConfig: sample_interval_ms, sliding_window_ms, grow_factor (Ratio), shrink_factor (Ratio), would_block_grow_threshold, buffering_churn_threshold, mapping_error_threshold, high_memory_pressure_shrink_factor, starvation_priority_boost_factor, min_window_ms, min_window_bytes.
- AdaptiveBackpressureSnapshot: observed_fill_bytes_per_second, observed_drain_bytes_per_second, average_recovery_ms, would_block_count_recent, buffering_churn_count_recent, mapping_error_count_recent, downstream_starvation_detected, byte_time_mapping_unstable, current_window_too_small, current_window_too_large, urgent_read_demand.
- AdaptiveBackpressureGuard: window_upper_bound_ms, window_upper_bound_bytes, interleave_max_window_ms, interleave_max_window_bytes, live_max_window_ms, resource_max_bytes.

Rules:
- All grow/shrink is discrete per sample_interval.
- grow/shrink uses Ratio, NOT double.
- Diagnostics may expose double converted values.
- Window never exceeds guard.
- Urgent read cannot break ResourceBudget or MemoryPoolCritical.

STOP if:
- AdaptiveBackpressure uses double for grow/shrink, or window exceeds guard.

## P4G-047 BufferEngine Gate

Goal: Define BufferEngine with push/wake/snapshot and legal push matrix.

Evidence target:
- `backend/include/kivo/playback/control_plane/buffer/buffer_engine.hpp`
- `backend/include/kivo/playback/control_plane/buffer/buffer_snapshot.hpp`
- `artifacts/p4/evidence/p4_buffer_engine_check.json`

Required:
- BufferFenceKind enum: None, Seek, Flush, Stop, TrackSwitch, Recovery.
- BufferSnapshot: stamp, buffered_bytes, capacity_bytes, buffered_source_range (optional), buffered_time_range (optional), level (BufferLevel), eof_seen, live_edge_seen, buffer_has_pending_discontinuity, fence_kind.
- BufferPushStatus enum: Accepted, WouldBlockFull, RejectedStaleStamp, RejectedFenced, Cancelled.
- BufferPushResult: status.
- BufferWakeReason enum: Stop, Seek, Flush, IdleRelease, Shutdown, ResourceBudgetChanged.
- BufferEngine: push(item, policy), wake_all_waiters(reason), snapshot().

BufferPushResult legal matrix:
- Accepted: writes buffer yes, diagnostics optional, may block no.
- WouldBlockFull: writes buffer no, diagnostics yes, may block yes (cancellable).
- RejectedStaleStamp: writes buffer no, diagnostics stale counter, may block no.
- RejectedFenced: writes buffer no, diagnostics fence counter, may block no.
- Cancelled: writes buffer no, diagnostics optional, may block no.

STOP if:
- BufferEngine allows write on WouldBlockFull or RejectedStaleStamp.

## P4G-048 Timeline Gate

Goal: Define playback timeline state with confidence levels.

Evidence target:
- `backend/include/kivo/playback/control_plane/timeline/playback_timeline_state.hpp`
- `backend/include/kivo/playback/control_plane/timeline/source_timeline_state.hpp`

Required:
- SourceTimelineState: buffered_time_range (optional), buffered_source_range (optional), time_mapping_confidence, eof_seen, live_edge_seen.
- TimelinePositionConfidence enum: Unknown, CommandedEstimate, SourceMappedEstimate, DownstreamObserved, AudioClockStable.
- PlaybackTimelineState: session_id, revision, generation, state, commanded_position_ms, observed_position_ms, commanded_confidence, observed_confidence, source_timeline, duration_ms (optional), live_window (optional), pending_seek_target_ms (optional), playback_rate, is_seekable, is_buffering, is_eof, is_live_like, clock_confidence, observation_age, drift_report (optional).

Rules:
- SourceTimelineState belongs to current StreamRevision.
- StreamRevision increment clears eof_seen/live_edge_seen unless new reader confirms.
- Buffer flush clears buffer ranges.
- Buffer flush does NOT preserve old revision markers.
- P4 timeline is control-plane timeline.
- P4 does NOT own decoded frame timestamp.
- P4 does NOT own audio master clock.
- P6 may later promote AudioOutput clock to master.

STOP if:
- SourceTimelineState preserves eof_seen/live_edge_seen across StreamRevision increment.

## P4G-049 LiveWindow / LiveStreamPolicy Gate

Goal: Define live window observation and live stream policy.

Evidence target:
- `backend/include/kivo/playback/control_plane/live/live_window.hpp`
- `backend/include/kivo/playback/control_plane/live/live_stream_policy.hpp`

Required:
- LiveWindow: window_start_ms, window_end_ms, seekable_within_window, duration_dynamic.
- LiveWindowObservation: session_id, runtime_revision, based_on_timeline_revision, live_window, confidence, observed_at.
- LivePauseResumeBehavior enum: ResumeFromPausedPointIfStillInWindow, JumpToLiveEdgeOnResume, KeepLatencyTarget, PolicyDriven.
- LiveWindowExpiredBehavior enum: JumpToLiveEdge, ReportToUpperLayer, HardStop.
- LiveStreamPolicy: is_live_like, target_live_latency_ms, max_live_latency_ms, allow_seek_within_live_window, treat_live_edge_as_eof, pause_resume_behavior, window_expired_behavior.

Rules:
- P4 does NOT parse live manifest.
- P4 accepts LiveWindowObservation from P3/P5/P6 feedback.
- If LiveWindowObservation expired, pause resume policy must degrade by LiveWindowExpiredBehavior.

STOP if:
- P4 parses live manifest, or LiveStreamPolicy lacks window_expired_behavior.

## P4G-050 Pause / Idle / Live Policy Gate

Goal: Define pause/idle/live read decision resolution.

Evidence target:
- `backend/include/kivo/playback/control_plane/idle/pause_live_read_policy.hpp`

Required:
- ConsumerActivityKind enum: DataPulled, WouldBlockPoll, DemandHint, ClockObservation, DownstreamAck.
- PauseLiveReadDecision: continue_reading, chase_live_edge, allow_idle_release, reason_code (StableReasonCode).
- resolve_pause_live_read_policy(pause_policy, live_policy, idle_policy) function.

Rules:
- WouldBlockPoll only refreshes idle when state=Playing/Buffering or under rate-limited threshold.
- High-frequency WouldBlock polling must NOT prevent idle release forever.
- Idle release must wake all pipeline waiters.

STOP if:
- High-frequency WouldBlock polling prevents idle release forever.

## P4G-051 Recovery Gate

Goal: Define recovery context, decision, budget, and authorized fallback.

Evidence target:
- `backend/include/kivo/playback/control_plane/recovery/recovery_context.hpp`
- `backend/include/kivo/playback/control_plane/recovery/recovery_budget.hpp`
- `backend/include/kivo/playback/control_plane/recovery/authorized_fallback.hpp`
- `artifacts/p4/evidence/p4_recovery_check.json`

Required:
- RecoveryContext: recovery_epoch, root_failure (PlaybackFailure), state_before_recovery, decision (RecoveryDecision), started_at, associated_stamp.
- RecoveryDecisionResult: kind (RecoveryDecision), recovery_epoch, state_before_recovery, target_state, fallback (optional), backoff_delay_ms, requires_reader_replace, requires_stream_revision_increment, requires_buffer_flush, used_destructive_cancel, stable_reason_code.
- RecoveryBudget: max_retry_count, max_reopen_count, max_reader_replace_count, max_fallback_count, max_total_recovery_ms, retry_backoff_initial_ms, retry_backoff_max_ms, graceful_cancel_timeout_ms, destructive_cancel_timeout_ms, allow_fallback, allow_destructive_cancel.
- RecoveryBudgetCounter: failure_kind, retry_count, reopen_count, replace_count, fallback_count.
- AuthorizedFallback: fallback_id, authorization_kind (FallbackAuthorizationKind), candidate (P3SelectedCandidateRef), reader_factory (P3ReaderFactoryRef), probe_evidence (P3ProbeEvidenceView), stream_layout_hint, interleave_profile.

STOP if:
- RecoveryBudget lacks max limits or allows unbounded retry/fallback.

## P4G-052 Failure Taxonomy Gate

Goal: Define failure taxonomy with origin, severity, and policy.

Evidence target:
- `backend/include/kivo/playback/control_plane/recovery/playback_failure.hpp`
- `backend/include/kivo/playback/control_plane/recovery/playback_failure_kind.hpp`
- `backend/include/kivo/playback/control_plane/recovery/failure_taxonomy_policy.hpp`

Required:
- FailureOrigin enum: Reader, Buffer, Pipeline, Downstream, ControlThread, MemoryPool, RuntimePolicy.
- FailureSeverity enum: Info, Recoverable, UserVisible, Fatal.
- PlaybackFailureKind enum (authority: recovery/playback_failure_kind.hpp).
- PlaybackFailure: kind, severity, origin, stable_code (StableErrorCode), recoverable, user_visible, retry_after_ms (optional), diagnostic (optional SafeDiagnosticString).
- FailureClassificationRule: kind, severity, recoverable, user_visible, stable_code_prefix.
- FailureTaxonomyPolicy: expose_network_errors_to_user, classify_tls_failure_as_fatal, classify_drm_failure_as_user_visible, classify_low_speed_as_recoverable, default_severity_for_unknown, rules (RuleTableRef).

Rules:
- Hot path does NOT use std::vector dynamic allocation.
- Rule table uses fixed table reference.

STOP if:
- FailureTaxonomyPolicy uses std::vector in hot path.

## P4G-053 DRM / License Failure Policy Gate

Goal: Define DRM/license downstream failure path.

Evidence target:
- `backend/include/kivo/playback/control_plane/recovery/drm_failure_policy.hpp`

Rules:
- P4 does NOT acquire license.
- P4 does NOT decrypt.
- P4 stops scheduling new read if policy requires.
- P4 publishes ErrorRaised.
- RecoveryDecision default = HardTerminateToUpperLayer.
- If RecoveryPlan has authorized LicenseRefreshed fallback, P4 may switch fallback.

P4 must NOT contain:
- license_request
- license_server_call
- decrypt_sample
- DRM key material

STOP if:
- P4 contains license_request, license_server_call, decrypt_sample, or DRM key material.

## P4G-054 Shutdown Quarantine Gate

Goal: Define shutdown quarantine contract for safe reader lifetime.

Evidence target:
- `backend/include/kivo/playback/control_plane/quarantine/quarantined_reader_record.hpp`
- `backend/include/kivo/playback/control_plane/lifetime/shutdown_guard.hpp`
- `artifacts/p4/evidence/p4_quarantine_check.json`

Required:
- QuarantinedReaderRecord: reader_id, session_id, invalidated_stamp, reader (shared_ptr<IPlaybackReader>), lifetime_anchor (shared_ptr<ReaderWorkerLifetimeAnchor>), quarantined_at, root_failure, callbacks_permanently_ignored.
- ShutdownGuard: request_stop(), force_invalidate_all(), wake_all_waiters(), cancel_all_workers(), quarantine_unresponsive_workers(), destructive_cancel_if_timeout(), wait_for_quiescence(policy).

Rules:
- Quarantined reader cannot access PlaybackSession.
- All callbacks go to no-op sink.
- Quarantined worker does NOT block session TERMINATED if policy allows quarantine.
- QuarantineRegistry holds lifetime anchor.
- Session can terminate while quarantined reader naturally drains.
- Process shutdown may report leaked quarantine if reader never returns.
- Quarantine no-op callback must still count diagnostics, but cannot touch session state.
- ShutdownGuard does NOT accept SessionGeneration. Generation is assigned only by Control Thread via begin_stop_barrier().

STOP if:
- Quarantined reader can access PlaybackSession, or ShutdownGuard accepts SessionGeneration.

## P4G-055 EventBus Delivery Gate

Goal: Define EventBus with delivery class, capacity, coalescing, and callback reentrancy.

Evidence target:
- `backend/include/kivo/playback/control_plane/events/event_delivery_class.hpp`
- `backend/include/kivo/playback/control_plane/events/event_bus_policy.hpp`
- `backend/include/kivo/playback/control_plane/events/session_event.hpp`
- `artifacts/p4/evidence/p4_eventbus_check.json`

Required:
- EventDeliveryClass enum: Critical, State, Timeline, Diagnostics, Verbose.
- EventBusPolicy: max_pending_events_per_subscriber, coalesce_timeline_events, coalesce_diagnostics_events, drop_noncritical_when_full.
- SessionEvent: type, delivery_class, stamp (SnapshotStamp), command_id (optional), failure (optional), detail (optional EventDetail).
- EventDetail: command_superseded (optional), seek (optional), recovery (optional), track_switch_fence (optional), stream_end (optional).

Rules:
- Critical (Terminated/ErrorRaised/StopStarted/TrackSwitchFenceTimeout) must NOT be dropped.
- Timeline: may be coalesced.
- Diagnostics: may be coalesced.
- Verbose: may be sampled or dropped.

Callback reentrancy:
- Subscriber callback may call: snapshot(), timeline_snapshot(), diagnostics_snapshot(), submit_command_request().
- Subscriber callback must NOT synchronously call shutdown on same event delivery thread.
- If called: shutdown returns RejectedInvalidContext or schedules async stop intent.

No callback under lock:
- No subscriber callback under internal lock.
- No P5/P6/P7 callback under internal lock.
- No destruction of last shared_ptr<IPlaybackReader> under internal lock.
- No destruction of callback object under internal lock.
- No custom deleter execution under internal lock.

STOP if:
- Critical events can be dropped, or callback is invoked under internal lock.

## P4G-056 Queue Critical Lane Gate

Goal: Define control event queue with critical lane and overflow handling.

Evidence target:
- `backend/include/kivo/playback/control_plane/control/critical_event_queue.hpp`
- `artifacts/p4/evidence/p4_critical_queue_check.json`

Required:
- ControlEventQueuePolicy: normal_max_depth, critical_reserved_depth, coalesce_high_frequency_events.

Queue separation:
- CriticalEventQueue: Stop, Shutdown, Error, Recovery, RevisionBarrier, DownstreamFailure, DownstreamAck, TrackSwitchFenceTimeout.
- NormalEventQueue: BufferUpdated, DiagnosticsUpdated, DemandHint, ReadCompleted, TimelineUpdated.

Overflow rules:
- Normal queue full: coalesce latest by key, drop old non-critical if allowed, diagnostics counter++.
- Critical queue full: set atomic critical_overflow flag, wake Control Thread, producer must NOT block P5/P6 realtime path, diagnostics records overflow. If cannot recover, enter controlled shutdown / hard terminate.

Forbidden:
- Recursively generating new queue event in full path.

STOP if:
- Critical queue overflow blocks P5/P6 realtime path, or recursive event generation on full path.

## P4G-057 Diagnostics Gate

Goal: Define diagnostics with stable reason/error codes and no-heap histogram.

Evidence target:
- `backend/include/kivo/playback/control_plane/diagnostics/stable_reason_code.hpp`
- `backend/include/kivo/playback/control_plane/diagnostics/fixed_window_histogram.hpp`
- `backend/include/kivo/playback/control_plane/diagnostics/session_diagnostics.hpp`
- `backend/include/kivo/playback/control_plane/diagnostics/safe_diagnostic_string.hpp`

Required:
- StableReasonCode: value (U32). Hot path only.
- StableErrorCode: value (U32). Hot path only.
- SafeDiagnosticString: value (std::string), sensitivity (DiagnosticSensitivity), redacted (bool). Cold path only.
- DiagnosticSensitivity enum: PublicStableCode, RedactedUrl, HostHash, StatusCodeOnly, InternalOnly.
- FixedWindowHistogram<BucketCount, SlotCount>: slots (array), overflow_counts, slot_duration_ms, active_slot, active_slot_start. No heap allocation in hot path. O(1) or near O(1) update.
- SessionDiagnostics: session_id, total_bytes_read, total_bytes_delivered, total_bytes_discarded_as_stale, current_read_bytes_per_second, current_drain_bytes_per_second, read_latency_ms (histogram), pull_wait_ms (histogram), recovery_duration_ms (histogram), seek_count, seek_superseded_count, stale_read_discard_count, stale_callback_discard_count, stale_buffer_push_reject_count, cancel_requested_count, cancel_success_count, destructive_cancel_count, buffering_enter_count, buffering_churn_count, recovery_count, fallback_count, low_speed_violation_count, recovery_budget_counters (array), active_reader_instance_count, active_inflight_read_count, reader_last_byte_received_age_ms, reader_last_progress_age_ms, reader_stalled, read_expected_to_progress, command_queue_depth, control_event_queue_depth, critical_event_overflow_count, memory_pressure, buffer_snapshot, resource_budget, snapshot_stamp.

Rules:
- Hot path uses StableReasonCode/StableErrorCode, NOT std::string.
- Readable strings only in cold path table lookup.

STOP if:
- Hot path uses std::string for reason/error code, or FixedWindowHistogram allocates heap.

## P4G-058 ReaderStall / LowSpeed Gate

Goal: Define reader stall and low speed detection policies.

Evidence target:
- `backend/include/kivo/playback/control_plane/reader/low_speed_policy.hpp`
- `backend/include/kivo/playback/control_plane/reader/reader_stall_policy.hpp`

Required:
- LowSpeedPolicy: enable_low_speed_detection, min_bytes_per_second, observe_window_ms, violation_grace_ms, max_consecutive_violations, hysteresis_recover_ms.
- ReaderStallPolicy: enable_reader_stall_detection, max_no_byte_age_ms, max_no_progress_age_ms, escalate_to_recovery.

Rule:
- ReaderStall detection only active when: active read in-flight OR ReaderWorker executing blocking read OR downstream_starved AND read_expected_to_progress.

STOP if:
- ReaderStall detection is active when no read is in-flight and downstream is not starved.

## P4G-059 FlushScope / RevisionBarrier Table Gate

Goal: Define FlushScope revision table.

Evidence target:
- `backend/include/kivo/playback/control_plane/control/flush_scope.hpp`
- `artifacts/p4/evidence/p4_flush_scope_check.json`

Required table:

| FlushScope | cancel read | clear buffer | read_epoch++ | stream_revision++ | timeline_revision++ |
|---|---|---|---|---|---|
| BufferOnly | no | yes | no | no | maybe |
| ReaderInFlight | yes | maybe | yes | no | no |
| StreamRevision | yes | yes | yes | yes | yes |
| TimelinePending | no | no | no | no | yes |
| FullSession | yes | yes | yes | yes | yes |

Rules:
- SeekFence Internal Flush uses StreamRevision semantics.
- Stop uses FullSession semantics.
- FlushCommand target revision stale -> DuplicateIgnored or Coalesced.
- Flush must carry SeekEpoch/RecoveryEpoch when caused by seek or recovery.

STOP if:
- FlushScope table behavior is not enforced in tests.

## P4G-060 ResourceBudget Gate

Goal: Define resource budget with dynamic shrink events.

Evidence target:
- `backend/include/kivo/playback/control_plane/runtime/resource_budget.hpp`

Required:
- ResourceBudget: max_inflight_read_count, max_inflight_seek_count, max_reader_instance_count, max_inflight_read_bytes, max_buffer_bytes, max_memory_pool_bytes, max_command_queue_delay_ms, max_control_event_backlog, max_adaptive_window_ms, max_adaptive_window_bytes.
- ResourceBudgetEventKind enum: Updated, ShrinkStarted, ShrinkBlocked, ShrinkCompleted.
- ResourceBudgetDiagnostics: current_resource_budget, effective_resource_budget, resource_budget_shrink_pending.

STOP if:
- ResourceBudget lacks any max limit field.

## P4G-061 Output Boundaries to P5/P6/P7 Gate

Goal: Define P4 output boundary plans (not decode/render/audio results).

Evidence target:
- `backend/include/kivo/playback/control_plane/output_boundary/video_pipeline_input_plan.hpp`
- `backend/include/kivo/playback/control_plane/output_boundary/audio_pipeline_input_plan.hpp`
- `backend/include/kivo/playback/control_plane/output_boundary/subtitle_pipeline_input_plan.hpp`
- `artifacts/p4/evidence/p4_output_boundary_check.json`

Required:
- VideoPipelineInputPlan: session_id, stream_revision, selected_candidate, encoded_stream (shared_ptr<IMediaStreamPipeline>), layout_hint, interleave_profile, probe_evidence.
- AudioPipelineInputPlan: session_id, stream_revision, encoded_stream, timeline_snapshot, playback_rate, clock_handover_policy.
- SubtitlePipelineInputPlan: session_id, stream_revision, selected_candidate, timeline_snapshot, subtitle_availability.

P4 must NOT output:
- decoded frame, render surface, audio device, subtitle bitmap, UI event, DRM decrypted sample, segment scheduler, platform media session, AirPlay/Chromecast session.

STOP if:
- P4 output boundary contains decoded frame/render surface/audio device/subtitle bitmap.

## P4G-062 Negative Compile Tests Gate

Goal: Compile-fail tests for strong units and type authority.

Evidence target:
- `backend/tests/playback/control_plane/compile_fail/` directory
- `artifacts/p4/evidence/p4_negative_compile_check.json`

Required compile-fail tests:
1. Milliseconds cannot pass to Bytes.
2. Bytes cannot pass to Count.
3. BitsPerSecond cannot pass to Bytes.
4. BytesPerSecond cannot pass to BitsPerSecond.
5. PlaybackRateRatio cannot be constructed from double.
6. CommandId cannot pass to SessionId.
7. ReaderInstanceId cannot pass to MemoryBlockId.
8. External code cannot construct PlaybackCommand with internal sequence.
9. External code cannot construct CancellationToken directly.
10. P3 PlaybackHandoffContract cannot be redefined in P4 namespace.
11. P3 PlaybackCandidateSet cannot be redefined in P4 namespace.

Rules:
- Use dedicated negative compile targets.
- Each negative case must fail with expected diagnostic pattern.
- Negative tests must NOT be compiled into production target.

STOP if:
- Any negative compile test passes (should fail), or is compiled into production.

## P4G-063 Architecture Guard Gate

Goal: Machine-checkable architecture guard for P4 boundaries.

Evidence target:
- `backend/tools/p4/architecture_guard_scanner.py`
- `artifacts/p4/evidence/p4_architecture_guard_check.json`

Required scan:
- No P5/P6/P7/UI pollution in P4 headers/src.
- No P3 type redefinition in P4 namespace.
- No root manager bucket files.
- No forbidden implementation coupling tokens (D3D11, DXGI, Vulkan, WASAPI, AVAudioEngine, AAudio, AudioTrack, decoded_frame, render_surface, audio_device_open, subtitle_bitmap, QWidget, QML, QtQuick, DRM decrypt, license acquisition, provider_manager, source_manager, player_manager, god_player).
- Allowed only in: policy documentation, forbidden fixture, test sentinel.
- Forbidden scan must NOT misfire on business-qualified `*_engine.hpp` (buffer_engine.hpp, timeline_engine.hpp, recovery_engine.hpp, memory_pool_engine.hpp, event_bus_engine.hpp).

STOP if:
- Architecture guard cannot run in CI/blocking mode, or misfires on business-qualified engine files.

## P4G-064 Behavior Tests Gate

Goal: Comprehensive behavior tests covering all P4 subsystems.

Evidence target:
- `backend/tests/playback/control_plane/` test files
- `artifacts/p4/evidence/p4_behavior_tests_check.json`

Required behavior tests (from v1.8 section 67, 50 tests):
1. Duplicate enum scan.
2. Type ownership table completeness scan.
3. P3/P4 duplicate type boundary scan.
4. Multi-session stamp isolation.
5. SeekEpoch in RevisionStamp stale rejection.
6. RecoveryEpoch in RevisionStamp stale rejection.
7. DataOnly propagation.
8. DataAndEndMarker EOF propagation.
9. DataAndEndMarker LiveEdge propagation.
10. EndMarkerOnly EOF propagation.
11. EndMarkerOnly LiveEdge propagation.
12. StreamPullResult combination matrix.
13. Reader memory lease acquire/freeze/publish.
14. Reader cannot allocate encoded vector hot path.
15. ReadOnlyMemoryChain inline no-heap path.
16. ReadOnlyMemoryChain overflow uses arena/pool not heap.
17. MemoryBlock writable -> frozen -> borrowed -> retired.
18. Quarantined reader no-op callback sink.
19. Quarantined reader does not block session termination.
20. Critical queue full overflow flag.
21. DownstreamFailure survives queue pressure.
22. DownstreamAck survives queue pressure.
23. EventBus slow subscriber does not block Control Thread.
24. EventBus critical events not dropped.
25. Callback reentrant pause from BufferingStarted.
26. Callback shutdown same delivery thread rejected or async scheduled.
27. AudioOutput ClockObservation realtime safe no allocation.
28. TrackSwitch required ack complete.
29. TrackSwitch partial ack timeout.
30. Runtime policy stale revision rejected.
31. RuntimePolicyPayload one-kind-one-payload validation.
32. ResourceBudget shrink events.
33. FastDrain vs TokenBucket priority.
34. BufferingContext pause overrides pending Playing.
35. StartBufferDecision unified across initial/seek/recovery.
36. Paused seek warmup no BufferingStarted.
37. LiveWindowObservation expiry behavior.
38. LiveEdge projected as EOF event without changing StreamEndKind.
39. Stop during blocking pull wakes immediately.
40. Seek during BufferFull blocked push rejects old stamp.
41. P5 holds MemoryBlock across stop safely.
42. No callback under lock including shared_ptr deleter.
43. Negative compile tests for all strong units.
44. Lock-free RevisionValidator does not take SessionState lock.
45. ReaderWorker post queue full behavior.
46. Stop duplicate semantics.
47. FlushScope revision table behavior.
48. P4 output boundary contains no decoded frame/render/audio device/subtitle bitmap.
49. P4 architecture guard no P5/P6/P7/UI pollution.
50. ASan / TSan critical stress.

STOP if:
- Any of the 50 behavior tests is missing or fails.

## P4G-065 Soak Smoke Gate

Goal: 5-15 minute soak smoke test with fake reader and controlled clock.

Evidence target:
- `artifacts/p4/evidence/p4_soak_smoke_check.json`

Required smoke test scenarios:
- seek loop
- pause/resume loop
- reader invalidation
- runtime revision storm
- event queue pressure
- quarantine scenario
- evidence growth check

Duration: 5-15 minutes.

STOP if:
- Soak smoke test exceeds 15 minutes or fails any scenario.

## P4G-066 Full 4h Soak Gate

Goal: 4-hour full soak test with seeded randomized scenarios.

Evidence target:
- `artifacts/p4/evidence/p4_full_soak_check.json`

Required full soak scenarios:
- fake reader + fake clock
- memory growth threshold
- evidence growth threshold
- session error recovery
- reader invalidation
- manifest-like revision update
- playlist-like input switch
- seek storm
- stop/close stability
- eventbus slow subscriber
- critical queue pressure
- quarantine leak check

Duration: 4 hours.

Rules:
- P4 closure if NOT running full 4h soak must write `blocked`, must NOT write accepted_deferral again unless user explicitly authorizes.
- No memory growth beyond threshold.
- No evidence growth beyond threshold.
- No command queue leak.
- No reader leak.
- No session leak.

STOP if:
- Full 4h soak is not run and user has not authorized another deferral, or any leak is detected.

## P4G-067 Performance P99 Baseline Gate

Goal: Real P99 performance sampling for all P4 operations.

Evidence target:
- `artifacts/p4/evidence/p4_performance_p99_check.json`

Required P99 metrics:
- session_create_ms
- handoff_import_ms
- prepare_to_ready_ms
- start_to_first_read_ms
- seek_command_apply_ms
- flush_complete_ms
- pause_resume_ms
- stop_close_ms
- revision_apply_ms
- snapshot_read_ms
- evidence_sync_ms
- clock_observation_publish_us
- critical_queue_submit_us

Rules:
- P4 closure without real sampling must NOT write performance PASS.

STOP if:
- P99 baseline is not generated with real sampling, or any metric exceeds hard ceiling without timeout/retryable/probe_required output.

## P4G-068 ASan / TSan Stress Gate

Goal: AddressSanitizer and ThreadSanitizer critical stress tests.

Evidence target:
- `artifacts/p4/evidence/p4_asan_tsan_check.json`

Required stress scenarios:
- stale event flood
- critical queue pressure
- slow subscriber
- reader blocking
- stop
- seek storm
- revision storm
- quarantine leak
- memory lease reuse

Rules:
- ASan: no memory errors (UAF, buffer overflow, leak).
- TSan: no data races.

STOP if:
- ASan or TSan detects any error.

## P4G-069 Golden Evidence Pack Gate

Goal: Sanitized offline golden evidence pack for P4.

Evidence target:
- `backend/tests/golden_evidence/p4/golden_evidence_pack.json`
- `artifacts/p4/evidence/p4_golden_evidence_pack_check.json`

Required entries:
- session_create, handoff_import, seek, flush, stop, recovery, quarantine, track_switch, revision_barrier, memory_lease, buffer_push, eventbus_delivery, clock_observation, closure.

Rules:
- No raw token/cookie/signed URL/path/account/private host.
- No dynamic unstable fields unmasked.
- No current commit hash in evidence.

STOP if:
- Golden evidence pack contains raw secrets or current commit hash.

## P4G-070 Compatibility Matrix Gate

Goal: Document P4 runtime compatibility.

Evidence target:
- `artifacts/p4/evidence/p4_compatibility_matrix.json`

Required:
- delivery kind (progressive_file, progressive_http, hls, dash, playlist)
- tested with fake reader
- known limitations
- soak status
- performance status

STOP if:
- Known limitations are absent.

## P4G-071 Direct Executable Matrix Gate

Goal: Define direct executable evidence matrix.

Evidence target:
- `backend/architecture/p4_direct_executable_matrix.json`

Required entries:
- Each P4 test executable with name, command, working directory, expected result, evidence output, closure blocking, timeout.

STOP if:
- Any closure-blocking direct executable lacks evidence output.

## P4G-072 Closure Evidence Gate

Goal: Generate P4 closure evidence.

Evidence target:
- `artifacts/p4/evidence/P4-PLAYBACK-CONTROL-PLANE-V8-CLOSURE.json`

Required:
- schema `p4.closure.v8`
- commit model without current HEAD hash in committed evidence
- scope booleans (no_ui, no_decode, no_render, no_audio_output, no_drm_license)
- required check matrix (contract, behavior, stress, soak, performance, architecture_guard, evidence)
- closure gate results
- redaction_state sanitized
- accepted deferrals (if any, must be user-authorized)

STOP if:
- Evidence schema validation fails, or evidence tracks current commit hash.

## P4G-073 Closure Report Gate

Goal: Generate P4 closure report.

Evidence target:
- `docs/roadmap/p4_playback_control_plane_closure_report.md`

Required sections:
- summary, scope boundary, completed lines, test matrix, architecture guard, coverage, performance, compatibility, soak, golden evidence, seven gate results (contract/behavior/stress/soak/performance/architecture_guard/evidence), evidence files, redaction, secret audit, forbidden scan, cancellation, async, budget, revision, reader, memory, buffer, timeline, eventbus, clock, recovery, quarantine, track_switch, diagnostics, known_not_verified_items, accepted_deferrals, external final report pointer.

STOP if:
- known_not_verified_items or accepted_deferrals are incomplete.

## P4G-074 Risk Control Boundary Gate

Goal: Confirm each P4 family stays in its boundary.

Evidence target:
- `artifacts/p4/evidence/p4_risk_control_boundary_audit.json`

Rules:
- P4 session manages session, NOT plays.
- P4 reader reads bytes, NOT decodes.
- P4 buffer buffers items, NOT plays segment queue.
- P4 timeline tracks position, NOT owns decoded frame timestamp.
- P4 recovery recovers, NOT controls player.
- P4 eventbus delivers events, NOT executes playback.

STOP if:
- Any boundary rule is violated.

## P4G-075 P5/P6/P7 Handoff Boundary Gate

Goal: Define what P4 gives to P5/P6/P7 and what it does not.

Evidence target:
- `artifacts/p4/evidence/p4_p5_p6_p7_handoff_boundary.json`

Allowed P4 -> P5:
- VideoPipelineInputPlan, IMediaStreamPipeline, StreamLayoutHint, InterleaveProfile, ProbeEvidence, encoded stream.

Allowed P4 -> P6:
- AudioPipelineInputPlan, IMediaStreamPipeline, PlaybackTimelineState, PlaybackRateRatio, clock_handover_policy.

Allowed P4 -> P7:
- SubtitlePipelineInputPlan, PlaybackTimelineState, subtitle_availability.

Forbidden P4 -> P5/P6/P7:
- decoded frame, render surface, audio device, subtitle bitmap, UI event, DRM decrypted sample, segment scheduler, platform media session, AirPlay/Chromecast session.

STOP if:
- P5/P6/P7 handoff includes playback-core implementation objects.

## P4G-076 Final Success Definition Gate

Goal: Verify P4 success definition is met.

Evidence target:
- `artifacts/p4/evidence/p4_final_success_definition.json`

Required:
- For supported playback input, P4 produces: session, command queue, state machine, revision stamp, reader lifecycle, memory lease/freeze/publish, buffer item stream, timeline state, backpressure, downstream feedback, clock observation, track switch fence, recovery, quarantine, eventbus, diagnostics, output boundary plans, execution card, closure evidence.

STOP if:
- Final success lacks evidence traceability.

## P4G-077 Final Wording Gate

Goal: Prevent broad product claims.

Evidence target:
- closure report final wording section.

Allowed wording:
- `P4 designs and verifies the Playback Control Plane.`
- `P4 owns playback session, commands, timeline, input flow, reader lifecycle, memory lease, revision, recovery, eventbus, diagnostics, and output boundaries.`
- `P4 does not own demux implementation, decode, render, audio device, subtitle bitmap, provider selection, or UI.`

Forbidden wording:
- `Kivo playback complete.`
- `decoder/render/audio/UI complete.`
- `all providers production complete.`
- `HLS/DASH playback engine complete.`
- `DRM playback complete.`
- `Infuse/Kodi parity or surpass achieved.`

STOP if:
- Final report contains broader product completion claims.

## P4G-078 Final Report To User Gate

Goal: Make executor handoff understandable.

Evidence target:
- assistant final response after executing P4 work.

Allowed:
- Summarize completed gates, evidence paths, tests, known deferrals, and next stage.
- State whether P4 is closed or blocked.

Forbidden:
- Claiming closure if any P4G gate is incomplete.
- Hiding accepted deferrals.

STOP if:
- Final response does not mention blocked gates.

---

## Closure Conditions (v1.8 section 71)

P4 closure must simultaneously satisfy:

1. **Contract Gate**: P4 contracts complete, P3 handoff import complete, P5/P6/P7 output boundaries complete, schema_version complete, optional semantics complete.
2. **Behavior Gate**: State machine, command queue, timeline, read loop, seek/flush/stop, revision consumer, recovery, quarantine, eventbus, clock slot all verified.
3. **Stress Gate**: Stale event flood, critical queue pressure, slow subscriber, reader blocking, stop, seek storm, revision storm, quarantine leak, memory lease reuse all pass.
4. **Soak Gate**: Soak smoke pass, full 4h soak pass or user-explicit blocked exception, no memory/evidence/queue/reader/session leak.
5. **Performance Gate**: P99 baseline generated, evidence sync <=5ms, clock observation realtime publish target met, snapshot read target measured, seek apply target measured, no uncontrolled perf deferral.
6. **Architecture Guard Gate**: No decode/demux/render/audio_output/subtitle_render/UI/provider_selection/P3 type redefinition/root manager bucket.
7. **Evidence Gate**: Closure evidence generated, golden evidence generated, evidence does not chase current commit hash, final HEAD only in final report, redaction clean, forbidden token classification clean.

If any fails: Classification = FAIL_P4_PLAYBACK_CONTROL_PLANE_V8.

## Final Wording (v1.8 section 73)

P4 designs and verifies the Playback Control Plane.
P4 owns playback session, commands, timeline, input flow, reader lifecycle, memory lease, revision, recovery, eventbus, diagnostics, and output boundaries.
P4 does not own demux implementation, decode, render, audio device, subtitle bitmap, provider selection, or UI.
