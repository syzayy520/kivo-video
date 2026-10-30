#include "video/playback_graph/local_media/local_media_source_registry.hpp"
#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {
namespace {

[[nodiscard]] std::int32_t stream_index_from_track_id(const std::uint64_t track_id) noexcept {
    return track_id > 0 ? static_cast<std::int32_t>(track_id - 1) : -1;
}

[[nodiscard]] std::string codec_for_stream(
    const std::int32_t stream_index,
    const std::int32_t* stream_indices,
    const std::string* stream_codecs,
    const std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        if (stream_indices[index] == stream_index) {
            return stream_codecs[index];
        }
    }
    return {};
}

}  // namespace

bool PlaybackSessionRuntime::try_open_local_media(const OpenRequest& request) noexcept {
    const auto path = local_media::LocalMediaSourceRegistry::instance().resolve(request.source_id);
    if (!path.has_value()) {
        return false;
    }

    if (!local_media_pipeline_.open(*path)) {
        return false;
    }
    rebuild_playlist_queue_for_local_media(*path);

    const auto& status = local_media_pipeline_.status();
    if (!local_media_p3_handoff_.build(request.source_id, status)) {
        local_media_pipeline_.close();
        local_media_p3_handoff_.reset();
        return false;
    }

    duration_ms_ = status.duration_ms;
    position_ms_ = 0;
    last_source_id_ = request.source_id;
    subtitle_fixture_source_allowed_ = false;
    initialize_local_media_track_inventory(status);
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

    const auto& handoff_status = local_media_p3_handoff_.status();
    if (!handoff_status.p4_open_envelope_created) {
        return false;
    }
    local_media_p3_handoff_.mark_consumed_by_p7();

    const std::int32_t audio_stream_index =
        audio_track_id_ == 0 ? -1 : stream_index_from_track_id(audio_track_id_);
    const std::string audio_codec =
        audio_stream_index >= 0
            ? codec_for_stream(audio_stream_index,
                               probe_status.audio_stream_indices,
                               probe_status.audio_stream_codecs,
                               probe_status.audio_stream_count)
            : std::string{};
    const std::int32_t subtitle_stream_index =
        subtitle_enabled_ && subtitle_track_id_ != 0
            ? stream_index_from_track_id(subtitle_track_id_)
            : -1;
    const std::string subtitle_codec =
        subtitle_stream_index >= 0
            ? codec_for_stream(subtitle_stream_index,
                               probe_status.subtitle_stream_indices,
                               probe_status.subtitle_stream_codecs,
                               probe_status.subtitle_stream_count)
            : std::string{};

    if (!local_media_continuous_.start(
            *path,
            local_media_pipeline_.video_stream_index(),
            probe_status.video_codec,
            audio_stream_index,
            audio_codec,
            subtitle_stream_index,
            subtitle_codec,
            subtitle_enabled_ ? subtitle_track_id_ : 0)) {
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
