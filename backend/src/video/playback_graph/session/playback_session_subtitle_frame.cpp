#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

SubtitleFrameSnapshot PlaybackSessionRuntime::query_subtitle_frame() const noexcept {
    SubtitleFrameSnapshot frame{};
    if (!policy_state_valid_) {
        return frame;
    }

    frame.frame_available = false;
    frame.primary = {};
    frame.secondary = {};
    frame.valid = true;
    return frame;
}

}  // namespace kivo::video::playback_graph::runtime