#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

#include <memory>
#include <utility>

#include "video/video_plane/runtime/local_media/local_video_ffmpeg_session.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#endif

namespace kivo::video::video_plane::runtime::local_media {

LocalVideoRuntime::LocalVideoRuntime() = default;

LocalVideoRuntime::~LocalVideoRuntime() {
    close();
}

bool LocalVideoRuntime::start(const LocalVideoRuntimeOpenRequest& request) noexcept {
    close();
    stats_ = {};
    stats_.active = true;
    media_path_ = request.media_path;
    video_stream_index_ = request.video_stream_index;
    video_codec_ = request.video_codec;
    audio_stream_index_ = request.audio_stream_index;
    audio_codec_ = request.audio_codec;
    subtitle_stream_index_ = request.subtitle_stream_index;
    subtitle_codec_ = request.subtitle_codec;
    discard_before_media_time_us_ =
        request.start_position_ms > 0 ? request.start_position_ms * 1000 : -1;
    callback_user_data_ = request.callback_user_data;
    audio_packet_ready_ = request.audio_packet_ready;
    subtitle_packet_ready_ = request.subtitle_packet_ready;

#ifndef KIVO_ENABLE_FFMPEG
    stats_.error = "ffmpeg disabled";
    stats_.active = false;
    return false;
#else
    if (media_path_.empty() || video_stream_index_ < 0 || video_codec_.empty()) {
        stats_.error = "invalid p5 local video open request";
        stats_.active = false;
        return false;
    }

    ffmpeg_session_ = std::make_unique<FfmpegSession>();
    using namespace kivo::cinema_engine;

    if (!ffmpeg_session_->source.open(media_path_)) {
        stats_.error = ffmpeg_session_->source.error().empty() ? "source open failed"
                                                               : ffmpeg_session_->source.error();
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }
    stats_.source_session_connected = true;
    stats_.source_provider_kind = ffmpeg_session_->source.provider_kind_name();

    FfmpegInputConfig input_config{};
    input_config.callbacks = ffmpeg_session_->source.ffmpeg_callbacks();
    FfmpegFormatHandle fmt_handle = FfmpegAdapter::open_custom_io(input_config);
    if (!fmt_handle.native) {
        stats_.error = ffmpeg_session_->source.error().empty()
                           ? "source-backed format open failed"
                           : ffmpeg_session_->source.error();
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }

    FfmpegCodecHandle codec_handle =
        FfmpegAdapter::create_codec_context(fmt_handle, video_stream_index_);
    if (!codec_handle.native) {
        FfmpegAdapter::free_format_context(fmt_handle);
        stats_.error = "video codec context failed";
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }

    if (!ffmpeg_session_->decoder.adopt_codec_context(
            codec_handle.native, video_codec_, "p5_local_video_decode")) {
        FfmpegAdapter::free_codec_context(codec_handle);
        FfmpegAdapter::free_format_context(fmt_handle);
        stats_.error = "video codec adopt failed";
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }

    if (audio_stream_index_ >= 0 && !audio_codec_.empty() &&
        request.audio_decoder_open != nullptr) {
        FfmpegCodecHandle audio_codec_handle =
            FfmpegAdapter::create_codec_context(fmt_handle, audio_stream_index_);
        if (audio_codec_handle.native != nullptr) {
            LocalVideoAudioDecoderOpen audio_request;
            audio_request.codec_context = audio_codec_handle.native;
            audio_request.codec_name = audio_codec_;
            audio_request.stream_index = audio_stream_index_;
            stats_.audio_decoder_open_dispatched =
                request.audio_decoder_open(callback_user_data_, std::move(audio_request));
            if (!stats_.audio_decoder_open_dispatched) {
                stats_.audio_error = "audio decoder open callback failed";
            }
        } else {
            stats_.audio_error = "audio codec context failed";
        }
    }

    if (request.start_position_ms > 0) {
        const double seconds = static_cast<double>(request.start_position_ms) / 1000.0;
        if (!FfmpegAdapter::seek(fmt_handle, seconds)) {
            FfmpegAdapter::free_format_context(fmt_handle);
            stats_.error = "initial demux seek failed";
            stats_.active = false;
            ffmpeg_session_.reset();
            return false;
        }
    }

    ffmpeg_session_->format_handle = fmt_handle;
    stats_.demux_started = true;
    stats_.video_decode_started = true;
    stats_.continuous_rendering = true;
    next_packet_index_ = 0;

    if (pump_packets(5000) <= 0 || !stats_.first_frame_decoded) {
        if (stats_.error.empty()) {
            stats_.error = "no video frame decoded";
        }
        const std::string error = stats_.error;
        close();
        stats_.error = error;
        stats_.active = false;
        return false;
    }
    return true;
#endif
}

}  // namespace kivo::video::video_plane::runtime::local_media
