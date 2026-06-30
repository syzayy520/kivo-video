#include "video/playback_graph/local_media/local_media_source_registry.hpp"
#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

bool PlaybackSessionRuntime::try_open_local_media(const OpenRequest& request) noexcept {
    const auto path = local_media::LocalMediaSourceRegistry::instance().resolve(request.source_id);
    if (!path.has_value()) {
        return false;
    }

    if (!local_media_pipeline_.open(*path)) {
        return false;
    }

    const auto& status = local_media_pipeline_.status();
    duration_ms_ = status.duration_ms;
    position_ms_ = 0;
    last_source_id_ = request.source_id;
    initialize_track_inventory(request.source_id);
    video_surface_.width = status.width;
    video_surface_.height = status.height;
    if (video_surface_.height > 0) {
        video_surface_.aspect_ratio =
            static_cast<double>(video_surface_.width) /
            static_cast<double>(video_surface_.height);
    }
    return true;
}

bool PlaybackSessionRuntime::try_start_local_media() noexcept {
    if (!local_media_pipeline_.is_active()) {
        return false;
    }

    const auto& probe_status = local_media_pipeline_.status();
    const auto path = local_media::LocalMediaSourceRegistry::instance().resolve(last_source_id_);
    if (!path.has_value()) {
        return false;
    }

    if (!local_media_continuous_.start(
            *path,
            local_media_pipeline_.video_stream_index(),
            probe_status.video_codec)) {
        return false;
    }

    const auto& status = local_media_continuous_.stats();
    video_surface_.ready = status.frame_rendered;
    video_surface_.width = probe_status.width;
    video_surface_.height = probe_status.height;
    if (video_surface_.height > 0) {
        video_surface_.aspect_ratio =
            static_cast<double>(video_surface_.width) /
            static_cast<double>(video_surface_.height);
    }
    if (status.frame_rendered) {
        video_surface_.render_state = VideoRenderAttachmentState::SurfaceAttached;
    }
    position_ms_ = status.position_ms;
    return status.frame_rendered;
}

}  // namespace kivo::video::playback_graph::runtime