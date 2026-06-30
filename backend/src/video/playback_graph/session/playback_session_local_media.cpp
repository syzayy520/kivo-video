#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

LocalMediaPlaybackQuery PlaybackSessionRuntime::query_local_media_playback() const noexcept {
    LocalMediaPlaybackQuery query{};
    const auto& status = local_media_pipeline_.status();
    query.active = status.active;
    query.source_opened = status.source_opened;
    query.demux_started = status.demux_started;
    query.video_decode_started = status.video_decode_started;
    query.first_frame_decoded = status.first_frame_decoded;
    query.d3d11_uploaded = status.d3d11_uploaded;
    query.frame_rendered = status.frame_rendered;
    query.container = status.container;
    query.video_codec = status.video_codec;
    query.error = status.error;
    query.duration_ms = status.duration_ms;
    query.width = status.width;
    query.height = status.height;

    const auto& frame = local_media_pipeline_.frame_buffer();
    query.frame_pixels_valid = frame.valid;
    if (frame.valid) {
        query.width = frame.width;
        query.height = frame.height;
        query.rgba8888 = frame.rgba8888;
    }
    return query;
}

}  // namespace kivo::video::playback_graph::runtime