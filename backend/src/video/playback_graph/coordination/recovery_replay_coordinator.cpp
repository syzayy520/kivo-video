#include "video/playback_graph/coordination/recovery_replay_coordinator.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] bool action_enabled(const RecoveryReplayPlan& plan,
                                  RecoveryReplayAction action) noexcept {
    switch (action) {
        case RecoveryReplayAction::Seek:
            return plan.seek;
        case RecoveryReplayAction::Transport:
            return plan.transport;
        case RecoveryReplayAction::TrackSwitch:
            return plan.track_switch;
    }
    return false;
}

}  // namespace

RecoveryReplayOutcome RecoveryReplayCoordinator::run_fake(const RecoveryReplayPlan& plan) noexcept {
    return run(plan, false, RecoveryReplayAction::Seek, RecoveryReplayPhase::Validate);
}

RecoveryReplayOutcome RecoveryReplayCoordinator::run_fake_with_failure(
    const RecoveryReplayPlan& plan,
    RecoveryReplayAction failed_action,
    RecoveryReplayPhase failed_phase) noexcept {
    return run(plan, true, failed_action, failed_phase);
}

RecoveryReplayOutcome RecoveryReplayCoordinator::run(const RecoveryReplayPlan& plan,
                                                     bool inject_failure,
                                                     RecoveryReplayAction failed_action,
                                                     RecoveryReplayPhase failed_phase) noexcept {
    RecoveryReplayOutcome outcome{};
    constexpr std::array<RecoveryReplayAction, 3> kOrder{
        RecoveryReplayAction::Seek,
        RecoveryReplayAction::Transport,
        RecoveryReplayAction::TrackSwitch,
    };

    for (const auto action : kOrder) {
        if (!action_enabled(plan, action)) {
            continue;
        }

        outcome.order[outcome.order_count++] = action;
        if (inject_failure && action == failed_action &&
            failed_phase == RecoveryReplayPhase::Validate) {
            outcome.rolled_back = true;
            outcome.rollback_count = outcome.order_count;
            outcome.error = PlaybackGraphError::RecoveryFailed;
            return outcome;
        }
        if (inject_failure && action == failed_action &&
            failed_phase == RecoveryReplayPhase::Prepare) {
            outcome.rolled_back = true;
            outcome.rollback_count = outcome.order_count;
            outcome.error = PlaybackGraphError::RecoveryFailed;
            return outcome;
        }
        if (inject_failure && action == failed_action &&
            failed_phase == RecoveryReplayPhase::Commit) {
            outcome.rolled_back = true;
            outcome.rollback_count = outcome.order_count;
            outcome.error = PlaybackGraphError::RecoveryFailed;
            return outcome;
        }
    }

    outcome.accepted = true;
    outcome.committed = true;
    return outcome;
}

}  // namespace kivo::video::playback_graph::runtime
