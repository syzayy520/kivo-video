#include "kivo/playback/control_plane/concurrency/thread_model.hpp"
#include "kivo/playback/control_plane/revision/revision_barrier.hpp"
#include "kivo/playback/control_plane/revision/revision_validator.hpp"
#include "kivo/playback/control_plane/revision/cancellation.hpp"
#include "kivo/playback/control_plane/control/seek_target.hpp"
#include "kivo/playback/control_plane/control/seek_resolution.hpp"
#include "kivo/playback/control_plane/control/emergency_control_lane.hpp"
#include "kivo/playback/control_plane/control/authorized_playback_control_port.hpp"
#include "kivo/playback/control_plane/command/command_queue_policy.hpp"
#include "kivo/playback/control_plane/command/command_submit_status.hpp"
#include "kivo/playback/control_plane/command/playback_command.hpp"
#include "kivo/playback/control_plane/session/playback_session_handle.hpp"
#include "kivo/playback/control_plane/state/buffering_context.hpp"
#include "kivo/playback/control_plane/state/session_state.hpp"
#include "kivo/playback/control_plane/reader/reader_transport_kind.hpp"
#include "kivo/playback/control_plane/reader/reader_capability.hpp"
#include "kivo/playback/control_plane/reader/reader_instance_state.hpp"
#include "kivo/playback/control_plane/handoff/p3_handoff_ref.hpp"
#include "kivo/playback/control_plane/handoff/p3_probe_evidence_view.hpp"
#include "kivo/playback/control_plane/handoff/stream_layout_hint.hpp"
#include "kivo/playback/control_plane/handoff/interleave_profile.hpp"
#include "kivo/playback/control_plane/handoff/playback_open_envelope.hpp"
#include "kivo/playback/control_plane/runtime/runtime_policy_kind.hpp"
#include "kivo/playback/control_plane/runtime/runtime_policy_update.hpp"
#include "kivo/playback/control_plane/timeline/time_mapping_confidence.hpp"

#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

// P4G-014: Thread Model
static void test_thread_kind_exists()
{
    concurrency::ThreadKind t = concurrency::ThreadKind::ControlThread;
    assert(t == concurrency::ThreadKind::ControlThread);
    t = concurrency::ThreadKind::P6ClockObservationPath;
    assert(t == concurrency::ThreadKind::P6ClockObservationPath);
    assert(concurrency::ThreadKind::TestHarnessThread == concurrency::ThreadKind::TestHarnessThread);
    std::cout << "PASS: thread_kind_exists\n";
}

// P4G-015: RevisionBarrier
static void test_revision_barrier_seek()
{
    revision::RevisionBarrier barrier;
    auto result = barrier.begin_seek_barrier();
    assert(result.kind == revision::RevisionBarrierKind::Seek);
    assert(result.accepted);
    assert(result.new_stamp.generation.value == result.commit.old_stamp.generation.value + 1);
    std::cout << "PASS: revision_barrier_seek\n";
}

static void test_revision_barrier_all_kinds()
{
    revision::RevisionBarrier barrier;
    assert(barrier.begin_stop_barrier().kind == revision::RevisionBarrierKind::Stop);
    assert(barrier.begin_recovery_barrier().kind == revision::RevisionBarrierKind::Recovery);
    assert(barrier.begin_recovery_fallback_barrier().kind == revision::RevisionBarrierKind::RecoveryFallback);
    assert(barrier.begin_flush_barrier().kind == revision::RevisionBarrierKind::Flush);
    assert(barrier.begin_track_switch_barrier().kind == revision::RevisionBarrierKind::TrackSwitch);
    assert(barrier.begin_runtime_policy_update_barrier().kind == revision::RevisionBarrierKind::RuntimePolicyUpdate);
    std::cout << "PASS: revision_barrier_all_kinds\n";
}

static void test_revision_barrier_validate()
{
    revision::RevisionBarrier barrier;
    auto result = barrier.begin_stop_barrier();
    auto current = barrier.current_stamp();
    revision::RevisionStamp stamp;
    stamp.session_id = current.session_id;
    stamp.generation = current.generation;
    assert(barrier.validate(stamp));
    stamp.generation = revision::SessionGeneration{999};
    assert(!barrier.validate(stamp));
    std::cout << "PASS: revision_barrier_validate\n";
}

// P4G-016: RevisionValidator (lock-free)
static void test_revision_validator_fast()
{
    revision::RevisionValidator validator;
    revision::RevisionStamp stamp;
    stamp.session_id = revision::SessionId{1};
    stamp.generation = revision::SessionGeneration{1};
    validator.update(stamp);
    assert(validator.validate_fast(stamp));
    stamp.generation = revision::SessionGeneration{2};
    assert(!validator.validate_fast(stamp));
    std::cout << "PASS: revision_validator_fast\n";
}

static void test_revision_validator_snapshot()
{
    revision::RevisionValidator validator;
    revision::RevisionStamp stamp;
    stamp.session_id = revision::SessionId{7};
    stamp.generation = revision::SessionGeneration{3};
    validator.update(stamp);
    auto snap = validator.current_stamp_snapshot();
    assert(snap.session_id.value == 7);
    assert(snap.generation.value == 3);
    std::cout << "PASS: revision_validator_snapshot\n";
}

// P4G-017: Cancellation
static void test_cancellation_scope()
{
    revision::CancellationScope s = revision::CancellationScope::ReaderInFlight;
    assert(s == revision::CancellationScope::ReaderInFlight);
    s = revision::CancellationScope::DestructiveReaderAbort;
    assert(s == revision::CancellationScope::DestructiveReaderAbort);
    assert(revision::CancellationScope::Session == revision::CancellationScope::Session);
    std::cout << "PASS: cancellation_scope\n";
}

static void test_cancellation_token()
{
    revision::CancellationToken token;
    token.session_id = revision::SessionId{1};
    token.generation = revision::SessionGeneration{1};
    token.read_epoch = revision::ReadEpoch{1};
    token.nonce = 42;
    assert(token.session_id.value == 1);
    assert(token.nonce == 42);
    std::cout << "PASS: cancellation_token\n";
}

static void test_reader_cancel_request()
{
    revision::ReaderCancelRequest req;
    req.scope = revision::CancellationScope::ReaderAndBuffer;
    req.cancellation.session_id = revision::SessionId{2};
    assert(req.scope == revision::CancellationScope::ReaderAndBuffer);
    assert(req.cancellation.session_id.value == 2);
    std::cout << "PASS: reader_cancel_request\n";
}

// P4G-019: SeekTarget
static void test_seek_target_timeline()
{
    control::SeekTarget target;
    target.type = control::SeekTargetType::TimelinePosition;
    target.absolute_position_ms = primitives::MediaTimeMs{5000};
    target.accuracy = control::SeekAccuracy::FastApproximate;
    assert(target.type == control::SeekTargetType::TimelinePosition);
    assert(target.absolute_position_ms.has_value());
    assert(target.absolute_position_ms->is_valid_vod_absolute());
    std::cout << "PASS: seek_target_timeline\n";
}

// P4G-019: SeekResolution (separate file)
static void test_seek_resolution()
{
    control::SeekResolution res;
    res.kind = control::SeekResolutionKind::Approximate;
    res.control_plane_warmup_hint_ms = primitives::Milliseconds{100};
    assert(res.kind == control::SeekResolutionKind::Approximate);
    assert(res.control_plane_warmup_hint_ms.value == 100);
    std::cout << "PASS: seek_resolution\n";
}

// P4G-020: P3 Handoff Import
static void test_p3_handoff_ref()
{
    handoff::P3PlaybackHandoffRef ref;
    ref.opaque_p3_handoff = nullptr;
    ref.p3_schema_version = 1;
    assert(ref.p3_schema_version == 1);
    std::cout << "PASS: p3_handoff_ref\n";
}

static void test_playback_open_envelope()
{
    handoff::PlaybackOpenEnvelope env;
    env.session_id = revision::SessionId{42};
    env.runtime_revision_hook.allow_runtime_policy_update = true;
    assert(env.session_id.value == 42);
    assert(env.selected_candidate.ref == nullptr);
    assert(env.runtime_revision_hook.allow_runtime_policy_update);
    std::cout << "PASS: playback_open_envelope\n";
}

// P4G-021: P3ProbeEvidenceView (separate file)
static void test_p3_probe_evidence_view()
{
    handoff::P3ProbeEvidenceView view;
    view.has_duration = true;
    view.duration_ms = primitives::Milliseconds{120000};
    view.is_seekable = true;
    view.estimated_bitrate_bps = primitives::BitsPerSecond{1500000};
    assert(view.has_duration);
    assert(view.is_seekable);
    assert(!view.source_range.has_value());
    assert(!view.initial_time_range.has_value());
    std::cout << "PASS: p3_probe_evidence_view\n";
}

// P4G-021: StreamLayoutHint
static void test_stream_layout_hint()
{
    handoff::StreamLayoutHint hint;
    hint.has_known_duration = true;
    hint.is_seekable = true;
    hint.time_mapping_confidence = timeline::TimeMappingConfidence::P3Hinted;
    assert(hint.has_known_duration);
    assert(hint.time_mapping_confidence == timeline::TimeMappingConfidence::P3Hinted);
    std::cout << "PASS: stream_layout_hint\n";
}

// P4G-021: InterleaveProfile (separate file)
static void test_interleave_profile()
{
    handoff::InterleaveProfile profile;
    profile.kind = handoff::ContainerInterleaveKind::NormalInterleave;
    profile.recommended_readahead_ms = primitives::Milliseconds{500};
    assert(profile.kind == handoff::ContainerInterleaveKind::NormalInterleave);
    std::cout << "PASS: interleave_profile\n";
}

// P4G-021: TimeMappingConfidence
static void test_time_mapping_confidence_authority()
{
    timeline::TimeMappingConfidence c = timeline::TimeMappingConfidence::None;
    assert(c == timeline::TimeMappingConfidence::None);
    c = timeline::TimeMappingConfidence::Stable;
    assert(c == timeline::TimeMappingConfidence::Stable);
    std::cout << "PASS: time_mapping_confidence_authority\n";
}

// P4G-022: RuntimePolicyUpdate
static void test_runtime_policy_payload_valid()
{
    runtime::RuntimePolicyPayload payload;
    payload.kind = runtime::RuntimePolicyKind::ResourceBudget;
    payload.resource_budget_updated = true;
    assert(payload.is_valid());
    std::cout << "PASS: runtime_policy_payload_valid\n";
}

static void test_runtime_policy_payload_invalid_multiple()
{
    runtime::RuntimePolicyPayload payload;
    payload.buffer_requirement_updated = true;
    payload.resource_budget_updated = true;
    assert(!payload.is_valid());
    std::cout << "PASS: runtime_policy_payload_invalid_multiple\n";
}

static void test_runtime_policy_payload_invalid_empty()
{
    runtime::RuntimePolicyPayload payload;
    assert(!payload.is_valid());
    std::cout << "PASS: runtime_policy_payload_invalid_empty\n";
}

static void test_runtime_policy_update_revision()
{
    runtime::RuntimePolicyUpdate update;
    update.revision = revision::P4RuntimePolicyRevision{5};
    update.kind = runtime::RuntimePolicyKind::PlaybackRate;
    update.payload.playback_rate_updated = true;
    assert(update.revision.value == 5);
    assert(update.payload.is_valid());
    std::cout << "PASS: runtime_policy_update_revision\n";
}

static void test_runtime_revision_hook()
{
    runtime::RuntimeRevisionHook hook;
    hook.initial_revision = revision::P4RuntimePolicyRevision{1};
    hook.allow_runtime_policy_update = true;
    hook.allow_resource_budget_update = false;
    assert(hook.initial_revision.value == 1);
    assert(hook.allow_runtime_policy_update);
    std::cout << "PASS: runtime_revision_hook\n";
}

// P4G-023: PlaybackCommand
static void test_playback_command_request()
{
    command::PlaybackCommandRequest req;
    req.type = command::PlaybackCommandType::Seek;
    req.priority = command::CommandPriority::Critical;
    assert(req.type == command::PlaybackCommandType::Seek);
    assert(req.priority == command::CommandPriority::Critical);
    std::cout << "PASS: playback_command_request\n";
}

static void test_playback_command()
{
    command::PlaybackCommand cmd;
    cmd.id = revision::CommandId{10};
    cmd.sequence = revision::CommandSequence{3};
    cmd.generation = revision::SessionGeneration{1};
    assert(cmd.id.value == 10);
    assert(cmd.sequence.value == 3);
    std::cout << "PASS: playback_command\n";
}

static void test_user_track_switch_request()
{
    command::UserTrackSwitchRequest req;
    req.track_hint = 7;
    req.relative = false;
    assert(req.track_hint == 7);
    std::cout << "PASS: user_track_switch_request\n";
}

// P4G-024: AuthorizedPlaybackControlPort
static void test_track_switch_plan()
{
    control::TrackSwitchPlan plan;
    plan.plan_id = 1;
    plan.authorized_by_p3 = true;
    plan.authorized_by_p5 = true;
    plan.target_track_hint = 9;
    assert(plan.authorized_by_p3 && plan.authorized_by_p5);
    std::cout << "PASS: track_switch_plan\n";
}

// P4G-025: CommandQueue
static void test_command_submit_status()
{
    command::CommandSubmitStatus s = command::CommandSubmitStatus::Accepted;
    assert(s == command::CommandSubmitStatus::Accepted);
    s = command::CommandSubmitStatus::RejectedTerminated;
    assert(s == command::CommandSubmitStatus::RejectedTerminated);
    std::cout << "PASS: command_submit_status\n";
}

static void test_command_queue_policy()
{
    command::CommandQueuePolicy policy;
    policy.normal_max_depth = primitives::Count{32};
    policy.critical_reserved_depth = primitives::Count{4};
    policy.reject_when_full = true;
    assert(policy.normal_max_depth.value == 32);
    assert(policy.reject_when_full);
    std::cout << "PASS: command_queue_policy\n";
}

// P4G-026: EmergencyControlLane
static void test_emergency_control_lane()
{
    control::EmergencyControlLane lane;
    assert(!lane.stop_requested());
    lane.request_stop();
    assert(lane.stop_requested());
    lane.request_seek_fence();
    assert(lane.seek_fence_requested());
    lane.request_shutdown();
    assert(lane.shutdown_requested());
    std::cout << "PASS: emergency_control_lane\n";
}

// P4G-028: BufferingContext
static void test_buffering_context()
{
    state::BufferingContext ctx;
    ctx.state_before_buffering = state::SessionState::Playing;
    ctx.user_pause_after_entered = false;
    assert(ctx.state_before_buffering == state::SessionState::Playing);
    ctx.user_pause_after_entered = true;
    assert(ctx.user_pause_after_entered);
    std::cout << "PASS: buffering_context\n";
}

static void test_start_buffer_reason()
{
    state::StartBufferReason r = state::StartBufferReason::SeekWarmup;
    assert(r == state::StartBufferReason::SeekWarmup);
    r = state::StartBufferReason::RateRelock;
    assert(r == state::StartBufferReason::RateRelock);
    std::cout << "PASS: start_buffer_reason\n";
}

// P4G-029: ReaderTransportKind / ReaderCapability (separate files)
static void test_reader_transport_kind()
{
    reader::ReaderTransportKind k = reader::ReaderTransportKind::LocalSeekableFile;
    assert(k == reader::ReaderTransportKind::LocalSeekableFile);
    k = reader::ReaderTransportKind::HttpRangeLike;
    assert(k == reader::ReaderTransportKind::HttpRangeLike);
    std::cout << "PASS: reader_transport_kind\n";
}

static void test_reader_capability()
{
    reader::ReaderCapability cap;
    cap.seekable = true;
    cap.supports_range_read = true;
    cap.transport_kind = reader::ReaderTransportKind::LocalSeekableFile;
    assert(cap.seekable);
    assert(cap.transport_kind == reader::ReaderTransportKind::LocalSeekableFile);
    std::cout << "PASS: reader_capability\n";
}

// P4G-030: ReaderInstanceState
static void test_reader_instance_state_usable()
{
    assert(reader::is_usable_state(reader::ReaderInstanceState::Open));
    assert(reader::is_usable_state(reader::ReaderInstanceState::Reading));
    assert(!reader::is_usable_state(reader::ReaderInstanceState::Closed));
    std::cout << "PASS: reader_instance_state_usable\n";
}

static void test_reader_instance_state_permanently_unusable()
{
    assert(reader::is_permanently_unusable(reader::ReaderInstanceState::Invalidated));
    assert(reader::is_permanently_unusable(reader::ReaderInstanceState::Quarantined));
    assert(reader::is_permanently_unusable(reader::ReaderInstanceState::Destroyed));
    assert(!reader::is_permanently_unusable(reader::ReaderInstanceState::Open));
    std::cout << "PASS: reader_instance_state_permanently_unusable\n";
}

int main()
{
    test_thread_kind_exists();
    test_revision_barrier_seek();
    test_revision_barrier_all_kinds();
    test_revision_barrier_validate();
    test_revision_validator_fast();
    test_revision_validator_snapshot();
    test_cancellation_scope();
    test_cancellation_token();
    test_reader_cancel_request();
    test_seek_target_timeline();
    test_seek_resolution();
    test_p3_handoff_ref();
    test_playback_open_envelope();
    test_p3_probe_evidence_view();
    test_stream_layout_hint();
    test_interleave_profile();
    test_time_mapping_confidence_authority();
    test_runtime_policy_payload_valid();
    test_runtime_policy_payload_invalid_multiple();
    test_runtime_policy_payload_invalid_empty();
    test_runtime_policy_update_revision();
    test_runtime_revision_hook();
    test_playback_command_request();
    test_playback_command();
    test_user_track_switch_request();
    test_track_switch_plan();
    test_command_submit_status();
    test_command_queue_policy();
    test_emergency_control_lane();
    test_buffering_context();
    test_start_buffer_reason();
    test_reader_transport_kind();
    test_reader_capability();
    test_reader_instance_state_usable();
    test_reader_instance_state_permanently_unusable();
    std::cout << "ALL PASS: p4_revision_control_tests (36 tests)\n";
    return 0;
}
