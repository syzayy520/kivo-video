#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

int PlaybackSessionRuntime::pump_local_media_playback(const int max_packets) noexcept {
    if (!local_media_continuous_.is_active()) {
        return 0;
    }

    const int new_frames = local_media_continuous_.pump_packets(max_packets);
    const auto& status = local_media_continuous_.stats();
    position_ms_ = status.position_ms;
    if (status.frame_rendered) {
        video_surface_.ready = true;
        video_surface_.render_state = VideoRenderAttachmentState::SurfaceAttached;
    }
    if (new_frames > 0) {
        publish_current_snapshot();
    }
    return new_frames;
}

}  // namespace kivo::video::playback_graph::runtime