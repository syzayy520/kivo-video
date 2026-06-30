#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

CommandToken PlaybackSessionRuntime::attach_video_surface(
    const VideoSurfaceBindingRequest& request) noexcept {
    if (request.native_handle == 0) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    if (state_machine_.state() == PlaybackGraphState::Closed ||
        state_machine_.state() == PlaybackGraphState::NotCreated) {
        return reject(PlaybackGraphError::InvalidState);
    }

    video_surface_.ready = true;
    video_surface_.width = request.viewport_width > 0 ? request.viewport_width : 1920;
    video_surface_.height = request.viewport_height > 0 ? request.viewport_height : 1080;
    video_surface_.aspect_ratio =
        video_surface_.height > 0
            ? static_cast<double>(video_surface_.width) /
                  static_cast<double>(video_surface_.height)
            : 0.0;
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceAttached;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

CommandToken PlaybackSessionRuntime::detach_video_surface() noexcept {
    if (video_surface_.render_state != VideoRenderAttachmentState::SurfaceAttached) {
        return reject(PlaybackGraphError::InvalidState);
    }
    video_surface_.ready = false;
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceDetached;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

CommandToken PlaybackSessionRuntime::release_video_surface() noexcept {
    video_surface_ = {};
    video_surface_.render_state = VideoRenderAttachmentState::SurfaceReleased;
    publish_current_snapshot();

    CommandToken token{};
    token.command_id.value = 1;
    return token;
}

VideoSurfaceSnapshot PlaybackSessionRuntime::query_video_surface() const noexcept {
    return video_surface_;
}

}  // namespace kivo::video::playback_graph::runtime