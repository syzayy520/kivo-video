#include "video/playback_graph/recovery/recovery_replay_behavior.hpp"

namespace kivo::video::playback_graph::runtime {

RecoveryReplayBehaviorOutcome RecoveryReplayBehavior::replay(
    const RecoveryReplayPlan& plan,
    bool seek_target_invalid,
    bool track_switch_rollback_failed,
    bool graph_can_hold_paused,
    TransportIntentKind latest_transport) noexcept {
    RecoveryReplayBehaviorOutcome outcome{};
    outcome.replayed_transport_intent = latest_transport;

    if (plan.seek && seek_target_invalid) {
        outcome.error = PlaybackGraphError::InvalidSeekTarget;
        if (graph_can_hold_paused) {
            outcome.paused_best_effort = true;
        } else {
            outcome.graph_failed = true;
        }
        return outcome;
    }

    if (plan.track_switch && track_switch_rollback_failed) {
        outcome.error = PlaybackGraphError::RecoveryFailed;
        if (graph_can_hold_paused) {
            outcome.paused_best_effort = true;
        } else {
            outcome.graph_failed = true;
        }
        return outcome;
    }

    outcome.accepted = true;
    return outcome;
}

}  // namespace kivo::video::playback_graph::runtime
