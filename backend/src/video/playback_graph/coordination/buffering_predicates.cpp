#include "video/playback_graph/coordination/buffering_predicates.hpp"

namespace kivo::video::playback_graph::runtime {

bool buffering_ready(const BufferingInputs& inputs) noexcept {
    const auto audio_ready = !inputs.audio_selected || inputs.audio_ready;
    const auto video_ready = !inputs.video_selected || inputs.video_ready;
    return audio_ready && video_ready;
}

bool rebuffering_needed(const BufferingInputs& inputs) noexcept {
    return !buffering_ready(inputs) && inputs.underflow_duration_ms >= inputs.debounce_ms;
}

bool stalled_predicate(const BufferingInputs& inputs) noexcept {
    if (inputs.live_source && !inputs.source_explicitly_terminated) {
        return inputs.underflow_duration_ms >= inputs.stalled_timeout_ms;
    }
    return !buffering_ready(inputs) && inputs.underflow_duration_ms >= inputs.stalled_timeout_ms;
}

}  // namespace kivo::video::playback_graph::runtime
