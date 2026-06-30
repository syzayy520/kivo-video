#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

LocalMediaPlaybackQuery PlaybackSessionRuntime::query_local_media_playback() const noexcept {
    LocalMediaPlaybackQuery query{};
    const auto& probe_status = local_media_pipeline_.status();
    const bool continuous_active = local_media_continuous_.is_active();
    const auto& continuous_stats = local_media_continuous_.stats();
    const auto& frame = continuous_active ? local_media_continuous_.frame_buffer()
                                          : local_media_pipeline_.frame_buffer();

    query.active = local_media_pipeline_.is_active() || continuous_active;
    query.source_opened = probe_status.source_opened;
    if (continuous_active) {
        query.demux_started = continuous_stats.demux_started;
        query.video_decode_started = continuous_stats.video_decode_started;
        query.first_frame_decoded = continuous_stats.first_frame_decoded;
        query.frame_rendered = continuous_stats.frame_rendered;
        query.continuous_rendering = continuous_stats.continuous_rendering;
        query.decoded_frame_count = continuous_stats.decoded_frame_count;
        query.rendered_frame_count = continuous_stats.rendered_frame_count;
        query.playback_elapsed_ms = continuous_stats.playback_elapsed_ms;
        query.last_frame_signature = continuous_stats.last_frame_signature;
        query.error = continuous_stats.error;
    } else {
        query.demux_started = probe_status.demux_started;
        query.video_decode_started = probe_status.video_decode_started;
        query.first_frame_decoded = probe_status.first_frame_decoded;
        query.frame_rendered = probe_status.frame_rendered;
        query.error = probe_status.error;
    }

    query.d3d11_uploaded = probe_status.d3d11_uploaded;
    query.container = probe_status.container;
    query.video_codec = probe_status.video_codec;
    query.duration_ms = probe_status.duration_ms;
    query.width = probe_status.width;
    query.height = probe_status.height;
    query.frame_pixels_valid = frame.valid;
    if (frame.valid) {
        query.width = frame.width;
        query.height = frame.height;
        query.rgba8888 = frame.rgba8888;
    }
    return query;
}

}  // namespace kivo::video::playback_graph::runtime