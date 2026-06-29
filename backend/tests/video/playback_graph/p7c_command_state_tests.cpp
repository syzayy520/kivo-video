#include <array>
#include <cstdint>

#include "kivo/video/playback_graph/playback_session.hpp"
#include "video/playback_graph/command/command_conflict_matrix.hpp"
#include "video/playback_graph/command/command_lifetime_registry.hpp"
#include "video/playback_graph/command/pending_transport_intent.hpp"
#include "video/playback_graph/coordination/recovery_replay_coordinator.hpp"
#include "video/playback_graph/p4_bridge/p4_state_sync_adapter.hpp"
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
    return start_token.accepted() && session.snapshot().state == PlaybackGraphState::Starting;
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
    if (!expect(verify_registry_lifetime())) {
        return 6;
    }
    if (!expect(verify_conflict_matrix())) {
        return 7;
    }
    if (!expect(verify_pending_and_replay())) {
        return 8;
    }
    return 0;
}
