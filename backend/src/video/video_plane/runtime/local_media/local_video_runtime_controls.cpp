#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

#include "video/video_plane/runtime/local_media/local_video_ffmpeg_session.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#endif

namespace kivo::video::video_plane::runtime::local_media {

void LocalVideoRuntime::select_subtitle_stream(const std::int32_t stream_index,
                                               const std::string& codec_name) noexcept {
    subtitle_stream_index_ = stream_index;
    subtitle_codec_ = codec_name;
}

bool LocalVideoRuntime::seek_to(const std::int64_t target_ms) noexcept {
#ifndef KIVO_ENABLE_FFMPEG
    (void)target_ms;
    return false;
#else
    if (!stats_.active || ffmpeg_session_ == nullptr || target_ms < 0) {
        return false;
    }

    const double seconds = static_cast<double>(target_ms) / 1000.0;
    if (!kivo::cinema_engine::FfmpegAdapter::seek(
            ffmpeg_session_->format_handle, seconds)) {
        stats_.error = ffmpeg_session_->source.error().empty() ? "demux seek failed"
                                                               : ffmpeg_session_->source.error();
        return false;
    }

    const auto flush = ffmpeg_session_->decoder.flush("p5_local_video_seek_flush");
    if (!flush.success) {
        stats_.error = flush.error_message.empty() ? "decoder flush failed"
                                                   : flush.error_message;
        return false;
    }

    frame_buffer_ = {};
    d3d11_upload_.reset_texture();
    discard_before_media_time_us_ = target_ms * 1000;
    return true;
#endif
}

void LocalVideoRuntime::close() noexcept {
#ifndef KIVO_ENABLE_FFMPEG
    stats_ = {};
    frame_buffer_ = {};
    d3d11_upload_.close();
    media_path_.clear();
    video_stream_index_ = -1;
    video_codec_.clear();
    audio_stream_index_ = -1;
    audio_codec_.clear();
    subtitle_stream_index_ = -1;
    subtitle_codec_.clear();
    discard_before_media_time_us_ = -1;
    callback_user_data_ = nullptr;
    audio_packet_ready_ = nullptr;
    subtitle_packet_ready_ = nullptr;
    next_packet_index_ = 0;
    ffmpeg_session_.reset();
    return;
#else
    if (ffmpeg_session_ != nullptr) {
        using namespace kivo::cinema_engine;

        ffmpeg_session_->decoder.close();
        FfmpegAdapter::free_format_context(ffmpeg_session_->format_handle);
        ffmpeg_session_->source.close();
    }
    ffmpeg_session_.reset();

    stats_ = {};
    frame_buffer_ = {};
    d3d11_upload_.close();
    media_path_.clear();
    video_stream_index_ = -1;
    video_codec_.clear();
    audio_stream_index_ = -1;
    audio_codec_.clear();
    subtitle_stream_index_ = -1;
    subtitle_codec_.clear();
    discard_before_media_time_us_ = -1;
    callback_user_data_ = nullptr;
    audio_packet_ready_ = nullptr;
    subtitle_packet_ready_ = nullptr;
    next_packet_index_ = 0;
#endif
}

}  // namespace kivo::video::video_plane::runtime::local_media
