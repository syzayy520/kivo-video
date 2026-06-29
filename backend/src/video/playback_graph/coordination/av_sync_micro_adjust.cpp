#include "video/playback_graph/coordination/av_sync_micro_adjust.hpp"

#include <cstdlib>

namespace kivo::video::playback_graph::runtime {

AvSyncDecision AvSyncMicroAdjust::decide(const AvSyncInputs& inputs,
                                         bool previous_capability_supported) noexcept {
    AvSyncDecision decision{};
    if (inputs.audio_selected) {
        decision.master = ClockMaster::Audio;
    } else if (inputs.video_selected) {
        decision.master = ClockMaster::Video;
    } else {
        decision.master = ClockMaster::External;
    }

    decision.capability_refreshed = true;
    decision.capability_changed = previous_capability_supported != inputs.p6_micro_adjust_supported;
    decision.rollback_required = previous_capability_supported && !inputs.p6_micro_adjust_supported;
    decision.micro_adjust_allowed =
        inputs.p6_micro_adjust_capability_known && inputs.p6_micro_adjust_supported &&
        std::abs(inputs.drift_ms) >= static_cast<int>(inputs.drift_threshold_ms);
    return decision;
}

}  // namespace kivo::video::playback_graph::runtime
