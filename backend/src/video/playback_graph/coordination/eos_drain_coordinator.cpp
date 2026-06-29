#include "video/playback_graph/coordination/eos_drain_coordinator.hpp"

namespace kivo::video::playback_graph::runtime {

EosDrainOutcome EosDrainCoordinator::drain_fake(const EosDrainPlan& plan,
                                                bool inject_last_frame_timeout,
                                                bool inject_output_drained_timeout) noexcept {
    EosDrainOutcome outcome{};
    outcome.required_bits = (plan.audio_selected ? 1U : 0U) + (plan.video_selected ? 1U : 0U) +
                            (plan.presentation_active ? 1U : 0U) + (plan.output_active ? 1U : 0U);
    if (inject_last_frame_timeout) {
        outcome.last_frame_timeout = true;
        outcome.error = PlaybackGraphError::DrainTimeout;
        return outcome;
    }
    if (inject_output_drained_timeout) {
        outcome.output_drained_timeout = true;
        outcome.error = PlaybackGraphError::DrainTimeout;
        return outcome;
    }
    outcome.completed_bits = outcome.required_bits;
    outcome.complete = true;
    return outcome;
}

}  // namespace kivo::video::playback_graph::runtime
