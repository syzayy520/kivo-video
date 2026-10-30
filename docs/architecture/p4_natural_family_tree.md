# P4 Playback Control Plane Natural Family Tree

Updated: 2026-06-28

## Purpose

This document defines the P4 directory family tree before any implementation files are created.

P4 follows the Kivo iron gate: concept -> planning design -> natural family tree -> implementation -> single file single responsibility -> no flat files.

## P4 Scope

P4 = Playback Control Plane.

P4 takes P3's selected playback input and converges it into a playback control plane.

P4 does NOT do: decode, demux implementation, PTS/DTS parsing, render, audio output, subtitle rendering, provider API, provider selection, FFmpeg probe, codec inspection, frame scheduling, A/V sync, cache, prefetch, UI, HDR, DRM decrypt, license acquisition.

## Family Tree

### Production Headers

```
backend/include/kivo/playback/control_plane/
  primitives/           # Strong-typed units (Milliseconds, Bytes, Count, etc.), Ratio, time types, source range
  contracts/            # StreamPullResult, StreamPullStatus, stream pipeline interface
  handoff/              # P3 handoff import refs/views, PlaybackOpenEnvelope, ProbeEvidenceView, StreamLayoutHint, InterleaveProfile
  session/              # PlaybackSessionHandle
  state/                # SessionState, BufferingContext, StartBufferDecision
  control/              # AuthorizedPlaybackControlPort, EmergencyControlLane, SeekTarget, SeekResolution, FlushScope, CriticalEventQueue
  command/              # PlaybackCommand, PlaybackCommandRequest, CommandQueuePolicy, CommandSubmitStatus
  revision/             # RevisionStamp, SnapshotStamp, CallbackToken, RevisionBarrier, RevisionValidator, Cancellation, RevisionMasterModel
  reader/               # ReaderTransportKind, ReaderCapability, ReaderInstanceState, ReaderReadResult, ReaderReadPayloadKind, ReaderOrchestrator, LowSpeedPolicy, ReaderStallPolicy
  worker/               # ReaderWorker
  buffer/               # BufferItem, BufferEngine, BufferSnapshot, BufferPushResult, BufferLevel, StreamEndKind, StreamEndMarker
  backpressure/         # BackpressureScheduler, AdaptiveBackpressure
  memory/               # MemoryLease, MutableMemoryBlock, ImmutableMemoryBlock, ReadOnlyMemoryChain, MemoryPoolPolicy, MemoryBlockState, MemoryPoolPressure
  timeline/             # PlaybackTimelineState, SourceTimelineState, TimeMappingConfidence
  clock/                # ClockObservation, ClockObservationSlot
  live/                 # LiveWindow, LiveStreamPolicy, LiveWindowObservation
  rate/                 # PlaybackRateRatio
  track/                # RequiredDownstreamAcks, TrackSwitchFence, DownstreamAckKind
  discontinuity/        # DiscontinuityReason, DiscontinuityMarker
  feedback/             # DownstreamFeedbackQueue, DownstreamDemandHint, DownstreamAck, DownstreamFailure
  recovery/             # RecoveryContext, RecoveryBudget, AuthorizedFallback, PlaybackFailure, PlaybackFailureKind, FailureTaxonomyPolicy, DrmFailurePolicy
  lifetime/             # ShutdownGuard
  quarantine/           # QuarantinedReaderRecord
  idle/                 # PauseLiveReadPolicy
  runtime/              # RuntimePolicyUpdate, RuntimePolicyPayload, RuntimePolicyKind, ResourceBudget
  diagnostics/          # StableReasonCode, StableErrorCode, FixedWindowHistogram, SessionDiagnostics, SafeDiagnosticString
  concurrency/          # ThreadModel
  events/               # EventDeliveryClass, EventBusPolicy, SessionEvent, EventDetail
  output_boundary/      # VideoPipelineInputPlan, AudioPipelineInputPlan, SubtitlePipelineInputPlan, IMediaStreamPipeline
  evidence/             # P4 evidence contracts
```

### Test Files

```
backend/tests/playback/control_plane/
  primitives/           # Strong unit tests, compile-fail tests
  contracts/            # StreamPullResult matrix tests
  handoff/              # P3 handoff import tests
  session/              # Session handle tests
  state/                # State machine adjacency tests, BufferingContext tests
  control/              # Command queue tests, emergency lane tests, seek tests, flush tests
  revision/             # RevisionStamp tests, RevisionBarrier tests, RevisionValidator tests, cancellation tests
  reader/               # ReaderReadResult matrix tests, reader state tests, reader stall tests
  worker/               # ReaderWorker tests
  buffer/               # BufferItem tests, BufferEngine tests, LiveEdge/EOF tests
  backpressure/         # Backpressure scheduler tests, adaptive tests
  memory/               # Memory lease/freeze/publish tests, ReadOnlyMemoryChain tests, MemoryPool tests
  timeline/             # Timeline tests
  clock/                # ClockObservationSlot realtime tests
  live/                 # LiveWindow/LiveStreamPolicy tests
  track/                # TrackSwitchFence tests, required ack tests
  discontinuity/        # Discontinuity tests
  feedback/             # DownstreamFeedback tests, clock observation tests
  recovery/             # Recovery tests, failure taxonomy tests, quarantine tests
  runtime/              # RuntimePolicyUpdate tests, ResourceBudget tests
  diagnostics/          # Diagnostics tests, histogram tests
  events/               # EventBus tests, critical queue tests
  output_boundary/      # P5/P6/P7 boundary tests
  compile_fail/         # Negative compile tests
  stress/               # ASan/TSan stress tests
```

### Architecture Files

```
backend/architecture/
  p4_architecture_policy.json
  p4_type_registry.json
  p4_direct_executable_matrix.json

backend/tools/p4/
  architecture_guard_scanner.py
```

## Single File Single Responsibility

Each header file owns exactly one type or one closely-related type group:

| File | Responsibility |
|------|---------------|
| primitives/value_types.hpp | Milliseconds, Microseconds, Nanoseconds, Bytes, BitsPerSecond, BytesPerSecond, Count |
| primitives/ratio.hpp | Ratio |
| rate/playback_rate_ratio.hpp | PlaybackRateRatio |
| revision/revision_master_model.hpp | All ID and Epoch types |
| revision/revision_stamp.hpp | RevisionStamp |
| revision/snapshot_stamp.hpp | SnapshotStamp |
| revision/callback_token.hpp | CallbackToken |
| revision/revision_barrier.hpp | RevisionBarrier, RevisionBarrierKind, RevisionBarrierResult, RevisionBarrierCommit |
| revision/revision_validator.hpp | RevisionValidator |
| revision/cancellation.hpp | CancellationToken, CancellationScope, ReaderCancelRequest |
| state/session_state.hpp | SessionState enum + is_valid_transition |
| state/buffering_context.hpp | BufferingContext, StartBufferReason, StartBufferDecision |
| buffer/stream_end_kind.hpp | StreamEndKind (sole authority) |
| buffer/stream_end_marker.hpp | StreamEndMarker |
| buffer/buffer_item.hpp | BufferItem, BufferItemKind |
| buffer/buffer_level.hpp | BufferLevel |
| buffer/buffer_snapshot.hpp | BufferSnapshot |
| buffer/buffer_engine.hpp | BufferEngine, BufferPushResult, BufferPushStatus, BufferFenceKind, BufferWakeReason |
| reader/reader_read_payload_kind.hpp | ReaderReadPayloadKind (sole authority) |
| reader/reader_read_result.hpp | ReaderReadResult, ReaderDataPayload |
| reader/reader_instance_state.hpp | ReaderInstanceState (sole authority) |
| reader/reader_transport_kind.hpp | ReaderTransportKind |
| reader/reader_capability.hpp | ReaderCapability |
| reader/reader_orchestrator.hpp | ReaderOrchestrator |
| reader/low_speed_policy.hpp | LowSpeedPolicy |
| reader/reader_stall_policy.hpp | ReaderStallPolicy |
| memory/memory_block_state.hpp | MemoryBlockState |
| memory/memory_pool_pressure.hpp | MemoryPoolPressure (sole authority) |
| memory/mutable_memory_block.hpp | MutableMemoryBlock |
| memory/mutable_block_lease.hpp | MutableBlockLease |
| memory/immutable_memory_block.hpp | ImmutableMemoryBlock |
| memory/memory_lease.hpp | MemoryLeaseStatus, MemoryLeaseResult, IReadBufferAllocator |
| memory/read_only_memory_chain.hpp | ReadOnlyMemorySegment, SegmentList, ReadOnlyMemoryChain |
| memory/memory_pool_policy.hpp | MemoryPoolStrategy, MemoryPoolPolicy |
| events/event_delivery_class.hpp | EventDeliveryClass (sole authority) |
| events/event_bus_policy.hpp | EventBusPolicy |
| events/session_event.hpp | SessionEvent, EventDetail, SessionEventType |
| clock/clock_observation.hpp | ClockObservation, ClockSource |
| clock/clock_observation_slot.hpp | ClockObservationSlot |
| track/downstream_ack_kind.hpp | DownstreamAckKind (sole authority) |
| track/required_downstream_acks.hpp | RequiredDownstreamAcks |
| track/track_switch_fence.hpp | TrackSwitchFence |
| track/track_switch_policy.hpp | TrackSwitchPolicy |
| track/authorized_track_switch_plan.hpp | AuthorizedTrackSwitchPlan, FallbackAuthorizationKind |
| discontinuity/discontinuity_reason.hpp | DiscontinuityReason (sole authority) |
| discontinuity/discontinuity_marker.hpp | DiscontinuityMarker |
| recovery/playback_failure_kind.hpp | PlaybackFailureKind (sole authority) |
| recovery/playback_failure.hpp | PlaybackFailure, FailureOrigin, FailureSeverity |
| recovery/failure_taxonomy_policy.hpp | FailureTaxonomyPolicy, FailureClassificationRule |
| recovery/recovery_context.hpp | RecoveryContext, RecoveryDecisionResult |
| recovery/recovery_budget.hpp | RecoveryBudget, RecoveryBudgetCounter |
| recovery/authorized_fallback.hpp | AuthorizedFallback |
| recovery/drm_failure_policy.hpp | DrmFailurePolicy |
| runtime/runtime_policy_kind.hpp | RuntimePolicyKind (sole authority) |
| runtime/runtime_policy_update.hpp | RuntimePolicyUpdate, RuntimePolicyPayload, RuntimeRevisionHook |
| runtime/resource_budget.hpp | ResourceBudget, ResourceBudgetEventKind, ResourceBudgetDiagnostics |
| diagnostics/stable_reason_code.hpp | StableReasonCode |
| diagnostics/stable_error_code.hpp | StableErrorCode |
| diagnostics/safe_diagnostic_string.hpp | SafeDiagnosticString, DiagnosticSensitivity |
| diagnostics/fixed_window_histogram.hpp | FixedWindowHistogram |
| diagnostics/session_diagnostics.hpp | SessionDiagnostics |
| primitives/stream_type.hpp | StreamType (sole authority) |
| primitives/time_types.hpp | MonotonicTimestamp, Deadline, ReadDeadline, PullDeadline, MediaTimeMs, MediaDeltaMs |
| primitives/source_range.hpp | SourceOffset, SourceRange |

## Forbidden Patterns

- `common.hpp`, `helper.hpp`, `utils.hpp`, `manager.hpp`, `types.hpp`, `pipeline.hpp` as root bucket
- `engine.hpp` as root bucket (but `*_engine.hpp` with business qualifier allowed)
- `god_player.hpp`, `player_manager.hpp`, `playback_manager.hpp`, `pipeline_manager.hpp`, `misc.hpp`, `all_contracts.hpp`
- Flat files in root or broad source directories
- P3 type redefinition in P4 namespace

## Dependency Direction

```
primitives <- handoff <- session <- state <- control <- command
primitives <- revision <- reader <- worker <- buffer <- backpressure <- memory
primitives <- timeline <- clock <- live <- rate
primitives <- feedback <- track <- discontinuity
primitives <- recovery <- lifetime <- quarantine <- idle
primitives <- runtime <- diagnostics <- concurrency <- events
all <- output_boundary <- evidence
```

- No layer may depend on decode/render/audio_output/ffmpeg/UI.
- P4 types must NOT redefine P3 sealed types.
- Tools are NOT runtime dependencies.
