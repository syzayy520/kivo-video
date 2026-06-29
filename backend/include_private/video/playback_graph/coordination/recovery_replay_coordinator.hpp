#pragma once

#include <array>
#include <cstdint>

#include "kivo/video/playback_graph/playback_graph_error.hpp"

namespace kivo::video::playback_graph::runtime {

enum class RecoveryReplayAction {
    Seek,
    Transport,
    TrackSwitch
};

enum class RecoveryReplayPhase {
    Validate,
    Prepare,
    Commit
};

struct RecoveryReplayPlan {
    bool seek{false};
    bool transport{false};
    bool track_switch{false};
};

struct RecoveryReplayOutcome {
    bool accepted{false};
    bool committed{false};
    bool rolled_back{false};
    PlaybackGraphError error{PlaybackGraphError::None};
    std::array<RecoveryReplayAction, 3> order{};
    std::uint32_t order_count{0};
    std::uint32_t rollback_count{0};
};

class RecoveryReplayCoordinator {
public:
    [[nodiscard]] RecoveryReplayOutcome run_fake(const RecoveryReplayPlan& plan) noexcept;
    [[nodiscard]] RecoveryReplayOutcome run_fake_with_failure(
        const RecoveryReplayPlan& plan,
        RecoveryReplayAction failed_action,
        RecoveryReplayPhase failed_phase) noexcept;

private:
    [[nodiscard]] RecoveryReplayOutcome run(const RecoveryReplayPlan& plan,
                                            bool inject_failure,
                                            RecoveryReplayAction failed_action,
                                            RecoveryReplayPhase failed_phase) noexcept;
};

}  // namespace kivo::video::playback_graph::runtime
