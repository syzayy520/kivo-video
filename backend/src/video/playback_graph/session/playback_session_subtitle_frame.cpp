#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

SubtitleFrameSnapshot PlaybackSessionRuntime::query_subtitle_frame() const noexcept {
    if (!policy_state_valid_) {
        return {};
    }

    if (local_media_continuous_.is_active()) {
        return local_media_continuous_.query_subtitle_frame(
            current_position_ms(), subtitle_delay_ms_, subtitle_track_id_, subtitle_enabled_);
    }

    return subtitle_frame_bridge_.query(current_position_ms(), subtitle_delay_ms_,
                                        subtitle_track_id_, subtitle_enabled_);
}

}  // namespace kivo::video::playback_graph::runtime
