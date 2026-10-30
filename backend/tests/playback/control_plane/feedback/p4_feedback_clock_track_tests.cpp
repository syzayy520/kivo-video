#include "kivo/playback/control_plane/feedback/downstream_feedback_queue.hpp"
#include "kivo/playback/control_plane/clock/clock_observation_slot.hpp"
#include "kivo/playback/control_plane/track/required_downstream_acks.hpp"
#include "kivo/playback/control_plane/track/track_switch_fence.hpp"
#include "kivo/playback/control_plane/discontinuity/discontinuity_reason.hpp"
#include "kivo/playback/control_plane/discontinuity/discontinuity_marker.hpp"
#include "kivo/playback/control_plane/backpressure/backpressure_scheduler.hpp"
#include "kivo/playback/control_plane/backpressure/adaptive_backpressure.hpp"
#include "kivo/playback/control_plane/buffer/buffer_snapshot.hpp"
#include "kivo/playback/control_plane/buffer/buffer_engine.hpp"
#include "kivo/playback/control_plane/timeline/source_timeline_state.hpp"
#include "kivo/playback/control_plane/timeline/playback_timeline_state.hpp"
#include "kivo/playback/control_plane/live/live_window.hpp"
#include "kivo/playback/control_plane/live/live_stream_policy.hpp"
#include "kivo/playback/control_plane/idle/pause_live_read_policy.hpp"

#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

// P4G-041: DownstreamFeedbackQueue
static void test_downstream_ack_kind()
{
    feedback::DownstreamAckKind k = feedback::DownstreamAckKind::SeekFenceAck;
    assert(k == feedback::DownstreamAckKind::SeekFenceAck);
    std::cout << "PASS: downstream_ack_kind\n";
}

static void test_downstream_demand_hint()
{
    feedback::DownstreamDemandHint hint;
    hint.id = revision::FeedbackId{1};
    hint.request.downstream_starved = true;
    assert(hint.id.value == 1);
    assert(hint.request.downstream_starved);
    std::cout << "PASS: downstream_demand_hint\n";
}

static void test_downstream_feedback_queue_policy()
{
    feedback::DownstreamFeedbackQueuePolicy p;
    p.normal_max_depth = primitives::Count{64};
    p.critical_reserved_depth = primitives::Count{8};
    p.coalesce_demand_hint = true;
    assert(p.normal_max_depth.value == 64);
    assert(p.coalesce_demand_hint);
    std::cout << "PASS: downstream_feedback_queue_policy\n";
}

// P4G-042: ClockObservationSlot
static void test_clock_source()
{
    clock::ClockSource s = clock::ClockSource::AudioOutput;
    assert(s == clock::ClockSource::AudioOutput);
    std::cout << "PASS: clock_source\n";
}

static void test_clock_observation()
{
    clock::ClockObservation obs;
    obs.session_id = revision::SessionId{1};
    obs.source = clock::ClockSource::AudioOutput;
    obs.playback_rate = primitives::Milliseconds{1000};
    assert(obs.source == clock::ClockSource::AudioOutput);
    assert(obs.drift_ms == std::nullopt);
    std::cout << "PASS: clock_observation\n";
}

// P4G-043: TrackSwitchFence
static void test_fallback_authorization_kind()
{
    track::FallbackAuthorizationKind k = track::FallbackAuthorizationKind::SameCredential;
    assert(k == track::FallbackAuthorizationKind::SameCredential);
    std::cout << "PASS: fallback_authorization_kind\n";
}

static void test_required_downstream_acks()
{
    track::RequiredDownstreamAcks acks;
    acks.kinds[0] = feedback::DownstreamAckKind::TrackSwitchFenceAck;
    acks.count = 1;
    assert(acks.count == 1);
    std::cout << "PASS: required_downstream_acks\n";
}

static void test_track_switch_fence_arm_and_ack()
{
    track::TrackSwitchFence fence;
    track::RequiredDownstreamAcks acks;
    acks.kinds[0] = feedback::DownstreamAckKind::TrackSwitchFenceAck;
    acks.kinds[1] = feedback::DownstreamAckKind::DiscontinuityAck;
    acks.count = 2;

    fence.arm(revision::TrackSwitchFenceId{1}, revision::StreamRevision{5}, acks, primitives::MonotonicTimestamp{1000});
    assert(fence.state == track::TrackSwitchFenceState::Armed);
    assert(!fence.all_required_ack_received());

    feedback::DownstreamAck ack;
    ack.stream_revision = revision::StreamRevision{5};
    ack.kind = feedback::DownstreamAckKind::TrackSwitchFenceAck;
    fence.submit_ack(ack);
    assert(fence.required_acks.count == 1);

    ack.kind = feedback::DownstreamAckKind::DiscontinuityAck;
    fence.submit_ack(ack);
    assert(fence.all_required_ack_received());
    std::cout << "PASS: track_switch_fence_arm_and_ack\n";
}

static void test_track_switch_fence_expired()
{
    track::TrackSwitchFence fence;
    track::RequiredDownstreamAcks acks;
    acks.count = 1;
    fence.arm(revision::TrackSwitchFenceId{1}, revision::StreamRevision{5}, acks, primitives::MonotonicTimestamp{1000});
    track::TrackSwitchPolicy policy;
    policy.fence_ack_timeout_ms = primitives::Milliseconds{500};
    assert(!fence.expired(primitives::MonotonicTimestamp{1200}, policy));
    assert(fence.expired(primitives::MonotonicTimestamp{1600}, policy));
    std::cout << "PASS: track_switch_fence_expired\n";
}

// P4G-044: Discontinuity
static void test_discontinuity_reason()
{
    discontinuity::DiscontinuityReason r = discontinuity::DiscontinuityReason::SeekSnapshot;
    assert(r == discontinuity::DiscontinuityReason::SeekSnapshot);
    r = discontinuity::DiscontinuityReason::LiveWindowJump;
    assert(r == discontinuity::DiscontinuityReason::LiveWindowJump);
    std::cout << "PASS: discontinuity_reason\n";
}

static void test_discontinuity_marker()
{
    discontinuity::DiscontinuityMarker m;
    m.reason = discontinuity::DiscontinuityReason::TrackSwitch;
    m.requires_downstream_ack = true;
    assert(m.reason == discontinuity::DiscontinuityReason::TrackSwitch);
    assert(m.requires_downstream_ack);
    std::cout << "PASS: discontinuity_marker\n";
}

// P4G-045: BackpressureScheduler
static void test_read_scheduling_decision()
{
    backpressure::ReadSchedulingDecision d;
    d.should_schedule_read = true;
    d.priority = 5;
    assert(d.should_schedule_read);
    assert(d.priority == 5);
    std::cout << "PASS: read_scheduling_decision\n";
}

static void test_token_debt_policy()
{
    backpressure::TokenDebtPolicy p;
    p.allow_urgent_token_debt = true;
    p.max_urgent_token_debt = primitives::Count{3};
    assert(p.allow_urgent_token_debt);
    std::cout << "PASS: token_debt_policy\n";
}

// P4G-046: AdaptiveBackpressure
static void test_adaptive_backpressure_config()
{
    backpressure::AdaptiveBackpressureConfig c;
    auto grow = primitives::Ratio::make(3, 2);
    auto shrink = primitives::Ratio::make(1, 2);
    assert(grow.has_value());
    assert(shrink.has_value());
    c.grow_factor = *grow;
    c.shrink_factor = *shrink;
    assert(c.grow_factor.numerator == 3);
    assert(c.shrink_factor.denominator == 2);
    std::cout << "PASS: adaptive_backpressure_config\n";
}

static void test_adaptive_backpressure_guard()
{
    backpressure::AdaptiveBackpressureGuard g;
    g.window_upper_bound_ms = primitives::Milliseconds{10000};
    g.window_upper_bound_bytes = primitives::Bytes{1048576};
    assert(g.window_upper_bound_ms.value == 10000);
    std::cout << "PASS: adaptive_backpressure_guard\n";
}

// P4G-047: BufferEngine
static void test_buffer_push_status()
{
    buffer::BufferPushStatus s = buffer::BufferPushStatus::Accepted;
    assert(s == buffer::BufferPushStatus::Accepted);
    s = buffer::BufferPushStatus::WouldBlockFull;
    assert(s == buffer::BufferPushStatus::WouldBlockFull);
    std::cout << "PASS: buffer_push_status\n";
}

static void test_buffer_snapshot()
{
    buffer::BufferSnapshot snap;
    snap.buffered_bytes = primitives::Bytes{1024};
    snap.capacity_bytes = primitives::Bytes{4096};
    snap.level = buffer::BufferLevel::Low;
    assert(snap.buffered_bytes.value == 1024);
    assert(snap.level == buffer::BufferLevel::Low);
    std::cout << "PASS: buffer_snapshot\n";
}

static void test_buffer_fence_kind()
{
    buffer::BufferFenceKind k = buffer::BufferFenceKind::Seek;
    assert(k == buffer::BufferFenceKind::Seek);
    k = buffer::BufferFenceKind::None;
    assert(k == buffer::BufferFenceKind::None);
    std::cout << "PASS: buffer_fence_kind\n";
}

// P4G-048: Timeline
static void test_timeline_position_confidence()
{
    timeline::TimelinePositionConfidence c = timeline::TimelinePositionConfidence::Unknown;
    assert(c == timeline::TimelinePositionConfidence::Unknown);
    c = timeline::TimelinePositionConfidence::AudioClockStable;
    assert(c == timeline::TimelinePositionConfidence::AudioClockStable);
    std::cout << "PASS: timeline_position_confidence\n";
}

static void test_source_timeline_state()
{
    timeline::SourceTimelineState s;
    assert(!s.eof_seen);
    assert(!s.live_edge_seen);
    assert(s.time_mapping_confidence == timeline::TimeMappingConfidence::None);
    std::cout << "PASS: source_timeline_state\n";
}

static void test_playback_timeline_state()
{
    timeline::PlaybackTimelineState ts;
    ts.state = state::SessionState::Playing;
    ts.commanded_position_ms = primitives::MediaTimeMs{5000};
    ts.is_seekable = true;
    assert(ts.state == state::SessionState::Playing);
    assert(ts.commanded_position_ms.is_valid_vod_absolute());
    assert(ts.is_seekable);
    std::cout << "PASS: playback_timeline_state\n";
}

// P4G-049: LiveWindow / LiveStreamPolicy
static void test_live_window()
{
    live::LiveWindow w;
    w.window_start_ms = primitives::MediaTimeMs{0};
    w.window_end_ms = primitives::MediaTimeMs{60000};
    w.seekable_within_window = true;
    assert(w.seekable_within_window);
    std::cout << "PASS: live_window\n";
}

static void test_live_stream_policy()
{
    live::LiveStreamPolicy p;
    p.is_live_like = true;
    p.treat_live_edge_as_eof = false;
    p.window_expired_behavior = live::LiveWindowExpiredBehavior::JumpToLiveEdge;
    assert(p.is_live_like);
    assert(p.window_expired_behavior == live::LiveWindowExpiredBehavior::JumpToLiveEdge);
    std::cout << "PASS: live_stream_policy\n";
}

// P4G-050: Pause/Idle/Live
static void test_consumer_activity_kind()
{
    idle::ConsumerActivityKind k = idle::ConsumerActivityKind::WouldBlockPoll;
    assert(k == idle::ConsumerActivityKind::WouldBlockPoll);
    std::cout << "PASS: consumer_activity_kind\n";
}

static void test_pause_live_read_decision_live()
{
    idle::PauseBufferPolicy pause;
    live::LiveStreamPolicy live;
    live.is_live_like = true;
    idle::IdleTimeoutPolicy idle_p;
    auto d = idle::resolve_pause_live_read_policy(pause, live, idle_p, idle::ConsumerActivityKind::DataPulled, true);
    assert(d.continue_reading);
    assert(d.chase_live_edge);
    std::cout << "PASS: pause_live_read_decision_live\n";
}

static void test_pause_live_read_decision_idle_release()
{
    idle::PauseBufferPolicy pause;
    pause.enable_pause_buffer = false;
    live::LiveStreamPolicy live;
    live.is_live_like = false;
    idle::IdleTimeoutPolicy idle_p;
    idle_p.enable_idle_release = true;
    // WouldBlockPoll when not playing/buffering -> allow idle release
    auto d = idle::resolve_pause_live_read_policy(pause, live, idle_p, idle::ConsumerActivityKind::WouldBlockPoll, false);
    assert(d.allow_idle_release);
    std::cout << "PASS: pause_live_read_decision_idle_release\n";
}

int main()
{
    test_downstream_ack_kind();
    test_downstream_demand_hint();
    test_downstream_feedback_queue_policy();
    test_clock_source();
    test_clock_observation();
    test_fallback_authorization_kind();
    test_required_downstream_acks();
    test_track_switch_fence_arm_and_ack();
    test_track_switch_fence_expired();
    test_discontinuity_reason();
    test_discontinuity_marker();
    test_read_scheduling_decision();
    test_token_debt_policy();
    test_adaptive_backpressure_config();
    test_adaptive_backpressure_guard();
    test_buffer_push_status();
    test_buffer_snapshot();
    test_buffer_fence_kind();
    test_timeline_position_confidence();
    test_source_timeline_state();
    test_playback_timeline_state();
    test_live_window();
    test_live_stream_policy();
    test_consumer_activity_kind();
    test_pause_live_read_decision_live();
    test_pause_live_read_decision_idle_release();
    std::cout << "ALL PASS: p4_feedback_clock_track_tests (26 tests)\n";
    return 0;
}
