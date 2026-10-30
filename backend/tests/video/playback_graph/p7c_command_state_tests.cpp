#include <array>
#include <cstdint>

#include "kivo/video/playback_graph/playback_session.hpp"
#include "video/playback_graph/command/command_conflict_matrix.hpp"
#include "video/playback_graph/command/command_lifetime_registry.hpp"
#include "video/playback_graph/command/pending_transport_intent.hpp"
#include "video/playback_graph/coordination/recovery_replay_coordinator.hpp"
#include "video/playback_graph/p4_bridge/p4_state_sync_adapter.hpp"
#include "video/playback_graph/p4_bridge/p4_timeline_contract_bridge.hpp"
#include "video/playback_graph/p8_bridge/subtitle_frame_p8_bridge.hpp"
#include "video/playback_graph/state/playback_graph_state_machine.hpp"

using namespace kivo::video::playback_graph;
using namespace kivo::video::playback_graph::runtime;

namespace {

[[nodiscard]] bool expect(bool condition) noexcept {
    return condition;
}

[[nodiscard]] GenerationStamp generation(std::uint64_t tuple) noexcept {
    GenerationStamp stamp{};
    stamp.tuple_id.value = tuple;
    stamp.stream_stamp_id = tuple;
    stamp.state = GenerationStampState::Active;
    return stamp;
}

[[nodiscard]] bool verify_closed_requires_closing() {
    constexpr std::array<PlaybackGraphState, 25> states{
        PlaybackGraphState::NotCreated,
        PlaybackGraphState::Created,
        PlaybackGraphState::Building,
        PlaybackGraphState::BuildFailed,
        PlaybackGraphState::Ready,
        PlaybackGraphState::Starting,
        PlaybackGraphState::Buffering,
        PlaybackGraphState::Playing,
        PlaybackGraphState::Rebuffering,
        PlaybackGraphState::Stalled,
        PlaybackGraphState::Pausing,
        PlaybackGraphState::Paused,
        PlaybackGraphState::Resuming,
        PlaybackGraphState::Seeking,
        PlaybackGraphState::Flushing,
        PlaybackGraphState::TrackSwitching,
        PlaybackGraphState::Draining,
        PlaybackGraphState::EosReached,
        PlaybackGraphState::SystemSuspending,
        PlaybackGraphState::SystemSuspended,
        PlaybackGraphState::SystemResuming,
        PlaybackGraphState::Recovering,
        PlaybackGraphState::Failed,
        PlaybackGraphState::Closing,
        PlaybackGraphState::Closed,
    };

    for (const auto state : states) {
        PlaybackGraphStateMachine machine{};
        if (state != PlaybackGraphState::NotCreated) {
            machine.transition_to(PlaybackGraphState::Closing);
            machine.transition_to(PlaybackGraphState::Closed);
            machine = PlaybackGraphStateMachine{};
        }

        if (state == PlaybackGraphState::Closing) {
            machine.transition_to(PlaybackGraphState::Closing);
            if (!machine.transition_to(PlaybackGraphState::Closed).accepted) {
                return false;
            }
            continue;
        }
        if (machine.transition_to(PlaybackGraphState::Closed).accepted) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool verify_build_failed_ownership() {
    PlaybackGraphStateMachine no_resource{};
    no_resource.transition_to(PlaybackGraphState::Building);
    const auto no_resource_result = no_resource.mark_build_failed(BuildFailureKind::NoResource);
    if (!no_resource_result.accepted ||
        no_resource_result.build_failure_kind != BuildFailureKind::NoResource) {
        return false;
    }
    if (no_resource.transition_to(PlaybackGraphState::Closed).accepted) {
        return false;
    }

    PlaybackGraphStateMachine partial{};
    partial.transition_to(PlaybackGraphState::Building);
    const auto partial_result = partial.mark_build_failed(BuildFailureKind::PartialResource);
    return partial_result.accepted &&
           partial_result.build_failure_kind == BuildFailureKind::PartialResource;
}

[[nodiscard]] bool verify_session_open_start_flow() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 7;
    const auto open_token = session.open(open);
    if (!open_token.accepted() || session.snapshot().state != PlaybackGraphState::Ready) {
        return false;
    }
    if (session.query_command(open_token.command_id).state != CommandLifecycleState::Completed) {
        return false;
    }

    PlaybackSession resume_probe{};
    OpenRequest resume_open{};
    resume_open.source_id = 8;
    resume_probe.open(resume_open);
    const auto resume_from_ready = resume_probe.resume();
    if (resume_from_ready.immediate_error != PlaybackGraphError::InvalidState) {
        return false;
    }

    const auto start_token = session.start();
    return start_token.accepted() && session.snapshot().state == PlaybackGraphState::Playing;
}

[[nodiscard]] bool verify_command_transaction_rollbacks() {
    P4StateSyncAdapter p4{};
    CommandLifetimeRegistry registry{2, 1};
    CommandCreateRequest request{};
    request.kind = GraphCommandKind::Open;
    request.requires_sidecar = true;

    p4.set_command_allocation_available(false);
    if (registry.create(request, p4).error != PlaybackGraphError::P4CommandCreateFailed ||
        registry.active_record_count() != 0) {
        return false;
    }
    p4.set_command_allocation_available(true);

    registry.set_sidecar_install_available(false);
    if (registry.create(request, p4).error != PlaybackGraphError::SidecarInstallFailed ||
        registry.active_record_count() != 0) {
        return false;
    }
    registry.set_sidecar_install_available(true);

    registry.set_queue_available(false);
    if (registry.create(request, p4).error != PlaybackGraphError::CommandQueueOverflow ||
        registry.active_record_count() != 0) {
        return false;
    }
    registry.set_queue_available(true);

    if (!registry.create(request, p4).accepted()) {
        return false;
    }
    if (registry.create(request, p4).error != PlaybackGraphError::CommandRegistryFull) {
        return false;
    }

    CommandCreateRequest close{};
    close.kind = GraphCommandKind::Close;
    close.requires_sidecar = false;
    return registry.create(close, p4).accepted();
}

[[nodiscard]] bool verify_p4_revision_boundary() {
    P4StateSyncAdapter adapter{};
    P4GraphTransitionInput input{};
    input.command_id = PlaybackCommandId{1};
    input.expected_generation = generation(1);
    input.observed_generation = generation(1);

    input.expected_playback_revision = PlaybackRevision{1};
    input.observed_playback_revision = PlaybackRevision{2};
    if (adapter.validate_and_commit(input).error != PlaybackGraphError::PlaybackRevisionMismatch) {
        return false;
    }

    input.expected_playback_revision = PlaybackRevision{};
    input.observed_playback_revision = PlaybackRevision{};
    input.expected_graph_snapshot_revision = GraphSnapshotRevision{3};
    input.observed_graph_snapshot_revision = GraphSnapshotRevision{4};
    if (adapter.validate_and_commit(input).error != PlaybackGraphError::GraphSnapshotMismatch) {
        return false;
    }

    input.expected_graph_snapshot_revision = GraphSnapshotRevision{};
    input.observed_graph_snapshot_revision = GraphSnapshotRevision{};
    input.observed_generation = generation(2);
    if (adapter.validate_and_commit(input).error != PlaybackGraphError::GenerationStampMismatch) {
        return false;
    }

    input.observed_generation = generation(1);
    return adapter.validate_and_commit(input).accepted &&
           adapter.current_playback_revision().value == 1;
}

[[nodiscard]] bool verify_p4_timeline_contract_bridge_truthful_ownership() {
    P4TimelineContractBridge bridge{};
    P4TimelineContractInput input{};
    input.session_id = PlaybackSessionId{1};
    input.playback_revision = PlaybackRevision{5};
    input.graph_snapshot_revision = GraphSnapshotRevision{11};
    input.graph_state = PlaybackGraphState::Playing;
    input.position_ms = 81027;
    input.duration_ms = 6435179;
    input.buffered_range_valid = true;
    input.buffered_start_ms = 81027;
    input.buffered_end_ms = 81147;
    input.continuous_playback_active = true;
    input.source_seekable = true;

    const auto snapshot = bridge.build(input);
    return snapshot.connected && snapshot.uses_p4_public_timeline_state &&
           !snapshot.p4_runtime_state_machine_owner &&
           snapshot.state.session_id.value == 1 &&
           snapshot.state.revision.value == 11 &&
           snapshot.state.generation.value == 5 &&
           snapshot.state.state ==
               kivo::playback::control_plane::state::SessionState::Playing &&
           snapshot.state.observed_position_ms.value == 81027 &&
           snapshot.state.duration_ms.has_value() &&
           snapshot.state.duration_ms->value == 6435179 &&
           snapshot.state.source_timeline.buffered_range.has_value();
}

[[nodiscard]] bool verify_registry_lifetime() {
    P4StateSyncAdapter p4{};
    CommandLifetimeRegistry registry{4, 1};
    CommandCreateRequest request{};
    request.kind = GraphCommandKind::Seek;
    request.requires_sidecar = true;
    const auto created = registry.create(request, p4);
    if (!created.accepted()) {
        return false;
    }
    if (!registry.has_sidecar(created.token.command_id)) {
        return false;
    }
    if (registry.remove_sidecar(created.token.command_id, RegistryThreadRole::PublicQueryThread)) {
        return false;
    }
    if (!registry.remove_sidecar(created.token.command_id, RegistryThreadRole::GraphOwnerThread)) {
        return false;
    }
    if (!registry.expire_to_tombstone(created.token.command_id) || registry.tombstone_count() != 1) {
        return false;
    }
    if (registry.query(created.token.command_id).state != CommandLifecycleState::Tombstoned) {
        return false;
    }
    if (registry.complete(created.token.command_id,
                          CommandTerminalStatus::Completed,
                          PlaybackGraphError::None)) {
        return false;
    }
    return registry.late_event_discard_count() == 1;
}

[[nodiscard]] bool verify_conflict_matrix() {
    if (command_conflict_matrix_rule_count() == 0) {
        return false;
    }
    if (resolve_command_conflict(GraphCommandKind::Start, PlaybackGraphState::Ready, false).decision !=
        CommandConflictDecision::Accept) {
        return false;
    }
    if (resolve_command_conflict(GraphCommandKind::Resume, PlaybackGraphState::Ready, false).decision !=
        CommandConflictDecision::RejectInvalidState) {
        return false;
    }
    if (resolve_command_conflict(GraphCommandKind::Pause, PlaybackGraphState::Starting, false).decision !=
        CommandConflictDecision::PendingTransportIntent) {
        return false;
    }
    if (resolve_command_conflict(GraphCommandKind::SetWindowScreenPolicy,
                                 PlaybackGraphState::NotCreated,
                                 false).decision != CommandConflictDecision::Accept) {
        return false;
    }
    return resolve_command_conflict(GraphCommandKind::Start, PlaybackGraphState::Closed, false).decision ==
           CommandConflictDecision::RejectClosedGraph;
}

[[nodiscard]] bool verify_pending_and_replay() {
    PendingTransportIntent intent{};
    intent.request(GraphCommandKind::Pause);
    intent.request(GraphCommandKind::Resume);
    if (!intent.has_pending() || intent.latest() != TransportIntentKind::Resume) {
        return false;
    }
    intent.clear_for_close();
    if (intent.has_pending()) {
        return false;
    }

    RecoveryReplayCoordinator replay{};
    RecoveryReplayPlan plan{};
    plan.seek = true;
    plan.transport = true;
    plan.track_switch = true;
    const auto outcome = replay.run_fake(plan);
    if (!outcome.committed || outcome.order_count != 3 ||
        outcome.order[0] != RecoveryReplayAction::Seek ||
        outcome.order[1] != RecoveryReplayAction::Transport ||
        outcome.order[2] != RecoveryReplayAction::TrackSwitch) {
        return false;
    }

    const auto failed = replay.run_fake_with_failure(plan,
                                                     RecoveryReplayAction::Transport,
                                                     RecoveryReplayPhase::Validate);
    return !failed.committed && failed.rolled_back && failed.order_count == 2 &&
           failed.error == PlaybackGraphError::RecoveryFailed;
}

[[nodiscard]] bool verify_stop_preserves_open_session() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 15;
    if (!session.open(open).accepted() || !session.start().accepted()) {
        return false;
    }
    if (!session.stop().accepted()) {
        return false;
    }
    if (session.snapshot().state != PlaybackGraphState::Ready ||
        session.snapshot().closed) {
        return false;
    }
    return session.start().accepted();
}

[[nodiscard]] bool verify_track_inventory_typed_contract() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 20;
    if (!session.open(open).accepted()) {
        return false;
    }

    const auto inventory = session.query_track_inventory();
    if (!inventory.valid || inventory.audio_track_count != 3 ||
        inventory.subtitle_track_count != 4) {
        return false;
    }
    if (!session.cycle_subtitle_track().accepted()) {
        return false;
    }
    const auto subtitle = session.query_subtitle();
    if (!subtitle.enabled || subtitle.selected_track_id != 1) {
        return false;
    }
    if (!session.cycle_audio_track().accepted()) {
        return false;
    }
    return session.query_track_inventory().selected_audio_track_id == 2;
}

[[nodiscard]] bool verify_ready_track_preselection_stays_ready() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 24;
    if (!session.open(open).accepted() ||
        session.snapshot().state != PlaybackGraphState::Ready) {
        return false;
    }

    SubtitleTrackSwitchRequest subtitle{};
    subtitle.track_id = 2;
    if (!session.switch_subtitle_track(subtitle).accepted() ||
        session.snapshot().state != PlaybackGraphState::Ready ||
        session.query_track_inventory().selected_subtitle_track_id != 2) {
        return false;
    }
    if (!session.cycle_audio_track().accepted() ||
        session.snapshot().state != PlaybackGraphState::Ready ||
        session.query_track_inventory().selected_audio_track_id != 2) {
        return false;
    }
    return session.start().accepted() &&
           session.snapshot().state == PlaybackGraphState::Playing;
}

[[nodiscard]] bool verify_subtitle_frame_typed_contract() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 21;
    if (!session.open(open).accepted()) {
        return false;
    }
    const auto without_subtitle = session.query_subtitle_frame();
    if (!without_subtitle.valid || without_subtitle.frame_available ||
        without_subtitle.runtime_connected ||
        without_subtitle.unavailable_reason != SubtitleFrameUnavailableReason::SubtitleDisabled) {
        return false;
    }

    if (!session.start().accepted()) {
        return false;
    }

    SubtitleTrackSwitchRequest track{};
    track.track_id = 2;
    if (!session.switch_subtitle_track(track).accepted()) {
        return false;
    }
    const auto bridged = session.query_subtitle_frame();
    return bridged.valid && bridged.runtime_connected && bridged.frame_available &&
           bridged.primary.primary_text[0] == 'c' && bridged.primary.primary_text[1] == 'u' &&
           bridged.primary.primary_text[2] == 'e';
}

[[nodiscard]] bool verify_p8_embedded_packet_bridge() {
    SubtitleFrameP8Bridge bridge{};
    bridge.attach(9, false);
    if (!bridge.active()) {
        return false;
    }

    EmbeddedSubtitlePacketSubmission text_packet;
    text_packet.start_time_us = 0;
    text_packet.end_time_us = 2'000'000;
    text_packet.codec_name = "subrip";
    text_packet.data = {'r', 'e', 'a', 'l'};
    if (!bridge.submit_embedded_packet(text_packet)) {
        return false;
    }

    const auto frame = bridge.query(1000, 0, 9, true);
    if (!frame.valid || !frame.runtime_connected || !frame.frame_available ||
        frame.primary.primary_text[0] != 'r' || frame.primary.primary_text[1] != 'e' ||
        frame.primary.primary_text[2] != 'a' || frame.primary.primary_text[3] != 'l') {
        return false;
    }

    SubtitleFrameP8Bridge pgs_bridge{};
    pgs_bridge.attach(10, false);
    EmbeddedSubtitlePacketSubmission pgs_packet;
    pgs_packet.start_time_us = 0;
    pgs_packet.end_time_us = 2'000'000;
    pgs_packet.codec_name = "hdmv_pgs_subtitle";
    pgs_packet.data = {0x50, 0x47};
    if (!pgs_bridge.submit_embedded_packet(pgs_packet)) {
        return false;
    }
    const auto pgs_frame = pgs_bridge.query(1000, 0, 10, true);
    const auto pgs_stats = pgs_bridge.stats();
    return pgs_frame.valid && pgs_frame.runtime_connected && !pgs_frame.frame_available &&
           pgs_stats.embedded_packet_ingest_connected &&
           pgs_stats.submitted_packet_count == 1 &&
           pgs_stats.unsupported_feature_count > 0 &&
           !pgs_stats.pgs_bitmap_render_connected;
}

[[nodiscard]] bool verify_diagnostics_invalid_closed() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 22;
    if (!session.open(open).accepted() || !session.close().accepted()) {
        return false;
    }
    const auto summary = session.query_diagnostics_summary();
    return !summary.valid &&
           summary.invalid_reason == DiagnosticsInvalidReason::GraphClosed;
}

[[nodiscard]] bool verify_window_screen_policy_contract() {
    PlaybackSession session{};
    if (!session.enter_fullscreen().accepted()) {
        return false;
    }
    auto window = session.query_window_screen_policy();
    if (!window.valid || window.mode != PlaybackWindowMode::Fullscreen) {
        return false;
    }
    ScreenMoveRequest move{};
    move.screen_index = 1;
    if (!session.move_to_screen(move).accepted()) {
        return false;
    }
    window = session.query_window_screen_policy();
    if (!window.valid || !window.screen_index_valid || window.screen_index != 1) {
        return false;
    }
    if (!session.exit_fullscreen().accepted()) {
        return false;
    }
    if (session.query_window_screen_policy().mode != PlaybackWindowMode::Windowed) {
        return false;
    }
    if (!session.close().accepted()) {
        return false;
    }
    return session.enter_fullscreen().immediate_error == PlaybackGraphError::ClosedGraph;
}

[[nodiscard]] bool verify_playlist_policy_contract() {
    PlaybackSession session{};
    OpenRequest open{};
    open.source_id = 23;
    if (!session.open(open).accepted()) {
        return false;
    }

    const auto initial = session.query_playlist_policy();
    if (!initial.valid || initial.queue_connected || initial.has_next ||
        initial.has_previous || initial.has_next_episode || initial.auto_play_next) {
        return false;
    }
    if (session.next().immediate_error !=
            PlaybackGraphError::PolicyUnsupportedByCapability ||
        session.previous().immediate_error !=
            PlaybackGraphError::PolicyUnsupportedByCapability ||
        session.play_next_episode().immediate_error !=
            PlaybackGraphError::PolicyUnsupportedByCapability) {
        return false;
    }
    AutoPlayNextRequest auto_play{};
    auto_play.enabled = true;
    if (!session.set_auto_play_next(auto_play).accepted()) {
        return false;
    }
    const auto enabled = session.query_playlist_policy();
    if (!enabled.valid || !enabled.auto_play_next || enabled.queue_connected ||
        enabled.item_count != 0) {
        return false;
    }
    if (!session.close().accepted()) {
        return false;
    }
    return !session.query_playlist_policy().valid &&
           session.set_auto_play_next(auto_play).immediate_error ==
               PlaybackGraphError::ClosedGraph;
}

}  // namespace

int main() {
    if (!expect(verify_closed_requires_closing())) {
        return 1;
    }
    if (!expect(verify_build_failed_ownership())) {
        return 2;
    }
    if (!expect(verify_session_open_start_flow())) {
        return 3;
    }
    if (!expect(verify_command_transaction_rollbacks())) {
        return 4;
    }
    if (!expect(verify_p4_revision_boundary())) {
        return 5;
    }
    if (!expect(verify_p4_timeline_contract_bridge_truthful_ownership())) {
        return 6;
    }
    if (!expect(verify_registry_lifetime())) {
        return 7;
    }
    if (!expect(verify_conflict_matrix())) {
        return 8;
    }
    if (!expect(verify_pending_and_replay())) {
        return 9;
    }
    if (!expect(verify_stop_preserves_open_session())) {
        return 10;
    }
    if (!expect(verify_track_inventory_typed_contract())) {
        return 11;
    }
    if (!expect(verify_ready_track_preselection_stays_ready())) {
        return 17;
    }
    if (!expect(verify_subtitle_frame_typed_contract())) {
        return 12;
    }
    if (!expect(verify_p8_embedded_packet_bridge())) {
        return 13;
    }
    if (!expect(verify_diagnostics_invalid_closed())) {
        return 14;
    }
    if (!expect(verify_window_screen_policy_contract())) {
        return 15;
    }
    if (!expect(verify_playlist_policy_contract())) {
        return 16;
    }
    return 0;
}
