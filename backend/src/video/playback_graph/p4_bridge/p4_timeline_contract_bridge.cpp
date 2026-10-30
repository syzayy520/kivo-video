#include "video/playback_graph/p4_bridge/p4_timeline_contract_bridge.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

namespace p4 = kivo::playback::control_plane;

[[nodiscard]] p4::state::SessionState map_state(
    const PlaybackGraphState state) noexcept {
    using P7State = PlaybackGraphState;
    using P4State = p4::state::SessionState;

    switch (state) {
    case P7State::NotCreated:
    case P7State::Created:
        return P4State::Idle;
    case P7State::Building:
    case P7State::Starting:
    case P7State::SystemResuming:
        return P4State::Initializing;
    case P7State::Ready:
    case P7State::EosReached:
        return P4State::Ready;
    case P7State::Playing:
    case P7State::Resuming:
    case P7State::TrackSwitching:
    case P7State::Draining:
        return P4State::Playing;
    case P7State::Pausing:
    case P7State::Paused:
    case P7State::SystemSuspended:
    case P7State::SystemSuspending:
        return P4State::Paused;
    case P7State::Seeking:
    case P7State::Flushing:
        return P4State::Seeking;
    case P7State::Buffering:
    case P7State::Rebuffering:
    case P7State::Stalled:
        return P4State::Buffering;
    case P7State::Recovering:
    case P7State::BuildFailed:
    case P7State::Failed:
        return P4State::ErrorRecovery;
    case P7State::Closing:
        return P4State::Stopping;
    case P7State::Closed:
        return P4State::Terminated;
    }
    return P4State::Idle;
}

[[nodiscard]] bool is_buffering_state(const PlaybackGraphState state) noexcept {
    return state == PlaybackGraphState::Buffering ||
           state == PlaybackGraphState::Rebuffering ||
           state == PlaybackGraphState::Stalled;
}

[[nodiscard]] std::uint64_t non_negative_ms(const std::int64_t value) noexcept {
    return value > 0 ? static_cast<std::uint64_t>(value) : 0U;
}

}  // namespace

P4TimelineContractSnapshot P4TimelineContractBridge::build(
    const P4TimelineContractInput& input) const noexcept {
    P4TimelineContractSnapshot snapshot{};
    snapshot.connected = input.session_id.value != 0;
    snapshot.uses_p4_public_timeline_state = snapshot.connected;
    snapshot.p4_runtime_state_machine_owner = false;

    auto& state = snapshot.state;
    state.session_id = p4::revision::SessionId{input.session_id.value};
    state.revision =
        p4::revision::TimelineRevision{input.graph_snapshot_revision.value};
    state.generation =
        p4::revision::SessionGeneration{input.playback_revision.value};
    state.state = map_state(input.graph_state);
    state.commanded_position_ms =
        p4::primitives::MediaTimeMs{input.seek_in_progress
                                        ? input.last_seek_target_ms
                                        : input.position_ms};
    state.observed_position_ms = p4::primitives::MediaTimeMs{input.position_ms};
    state.commanded_confidence =
        input.seek_in_progress
            ? p4::timeline::TimelinePositionConfidence::CommandedEstimate
            : p4::timeline::TimelinePositionConfidence::SourceMappedEstimate;
    state.observed_confidence =
        input.continuous_playback_active
            ? p4::timeline::TimelinePositionConfidence::DownstreamObserved
            : p4::timeline::TimelinePositionConfidence::SourceMappedEstimate;
    if (input.duration_ms > 0) {
        state.duration_ms =
            p4::primitives::Milliseconds{non_negative_ms(input.duration_ms)};
    }
    if (input.seek_in_progress) {
        state.pending_seek_target_ms =
            p4::primitives::MediaTimeMs{input.last_seek_target_ms};
    }
    state.is_seekable = input.source_seekable;
    state.is_buffering = is_buffering_state(input.graph_state);
    state.clock_confidence =
        input.continuous_playback_active
            ? p4::timeline::ClockConfidence::FreshObservation
            : p4::timeline::ClockConfidence::StaleObservation;
    state.playback_rate = p4::primitives::Milliseconds{1000U};

    if (input.buffered_range_valid) {
        p4::timeline::BufferedRange range{};
        range.buffered_time_begin =
            p4::primitives::MediaTimeMs{input.buffered_start_ms};
        range.buffered_time_end =
            p4::primitives::MediaTimeMs{input.buffered_end_ms};
        state.source_timeline.buffered_range = range;
        state.source_timeline.time_mapping_confidence =
            input.continuous_playback_active
                ? p4::timeline::TimeMappingConfidence::P5Observed
                : p4::timeline::TimeMappingConfidence::P3Hinted;
    }

    return snapshot;
}

}  // namespace kivo::video::playback_graph::runtime
