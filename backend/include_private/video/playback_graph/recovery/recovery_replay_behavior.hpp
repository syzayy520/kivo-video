#pragma once

#include "video/playback_graph/command/pending_transport_intent.hpp"
#include "video/playback_graph/coordination/recovery_replay_coordinator.hpp"

namespace kivo::video::playback_graph::runtime {

struct RecoveryReplayBehaviorOutcome {
    bool accepted{false};
    bool paused_best_effort{false};
    bool graph_failed{false};
    PlaybackGraphError error{PlaybackGraphError::None};
    TransportIntentKind replayed_transport_intent{TransportIntentKind::None};
};

class RecoveryReplayBehavior {
public:
    [[nodiscard]] RecoveryReplayBehaviorOutcome replay(const RecoveryReplayPlan& plan,
                                                       bool seek_target_invalid,
                                                       bool track_switch_rollback_failed,
                                                       bool graph_can_hold_paused,
                                                       TransportIntentKind latest_transport) noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
