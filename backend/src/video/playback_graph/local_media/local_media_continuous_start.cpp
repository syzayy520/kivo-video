#include "video/playback_graph/local_media/local_media_continuous_playback.hpp"

#include <utility>

#include "video/playback_graph/local_media/local_media_audio_dispatch.hpp"

namespace kivo::video::playback_graph::local_media {

bool LocalMediaContinuousPlayback::open_audio_decoder_callback(
    void* const user_data,
    LocalVideoAudioDecoderOpen&& request) noexcept {
    auto* self = static_cast<LocalMediaContinuousPlayback*>(user_data);
    return self != nullptr && self->open_audio_decoder(std::move(request));
}

void LocalMediaContinuousPlayback::audio_packet_callback(
    void* const user_data,
    LocalVideoAudioPacket&& packet) noexcept {
    auto* self = static_cast<LocalMediaContinuousPlayback*>(user_data);
    if (self != nullptr) {
        self->submit_audio_packet(std::move(packet));
    }
}

void LocalMediaContinuousPlayback::subtitle_packet_callback(
    void* const user_data,
    LocalVideoSubtitlePacket&& packet) noexcept {
    auto* self = static_cast<LocalMediaContinuousPlayback*>(user_data);
    if (self != nullptr) {
        self->submit_subtitle_packet(std::move(packet));
    }
}

bool LocalMediaContinuousPlayback::open_audio_decoder(
    LocalVideoAudioDecoderOpen&& request) noexcept {
    std::lock_guard lock(mutex_);
    kivo::video::audio_plane::runtime::local_pcm::LocalPcmAudioOpenRequest audio_request;
    audio_request.codec_context = request.codec_context;
    audio_request.codec_name = std::move(request.codec_name);
    audio_request.stream_index = request.stream_index;
    if (!audio_runtime_.open(std::move(audio_request))) {
        stats_.audio_error = audio_runtime_.stats().error;
        copy_audio_runtime_stats(audio_runtime_, stats_);
        return false;
    }
    copy_audio_runtime_stats(audio_runtime_, stats_);
    return true;
}

void LocalMediaContinuousPlayback::submit_audio_packet(LocalVideoAudioPacket&& packet) noexcept {
    std::lock_guard lock(mutex_);
    kivo::video::playback_graph::local_media::submit_audio_packet(
        audio_runtime_, std::move(packet), stats_);
}

void LocalMediaContinuousPlayback::submit_subtitle_packet(
    LocalVideoSubtitlePacket&& packet) noexcept {
    std::lock_guard lock(mutex_);
    kivo::video::playback_graph::runtime::EmbeddedSubtitlePacketSubmission submission;
    submission.start_time_us = packet.start_time_us;
    submission.end_time_us = packet.end_time_us;
    submission.key_packet = packet.is_key_frame;
    submission.codec_name = std::move(packet.codec_name);
    submission.data = std::move(packet.data);
    (void)subtitle_frame_bridge_.submit_embedded_packet(submission);
}

bool LocalMediaContinuousPlayback::start(
    const std::string& media_path,
    const int video_stream_index,
    const std::string& video_codec,
    const int audio_stream_index,
    const std::string& audio_codec,
    const int subtitle_stream_index,
    const std::string& subtitle_codec,
    const std::uint64_t subtitle_track_id,
    const std::int64_t start_position_ms) noexcept {
    close();

    {
        std::lock_guard lock(mutex_);
        stats_ = {};
        stats_.active = true;
        timeline_base_ms_ = start_position_ms > 0 ? start_position_ms : 0;
        paused_position_ms_ = timeline_base_ms_;
        paused_ = false;
        waiting_for_seek_frame_ = start_position_ms > 0;
        timeline_anchor_ = std::chrono::steady_clock::now();
        seek_preroll_anchor_ = timeline_anchor_;
        seek_preroll_anchor_valid_ = waiting_for_seek_frame_;
        stats_.p7_seek_preroll_active = waiting_for_seek_frame_;

        kivo::video::video_plane::runtime::local_media::LocalVideoRuntimeOpenRequest request;
        request.media_path = media_path;
        request.video_stream_index = video_stream_index;
        request.video_codec = video_codec;
        request.audio_stream_index = audio_stream_index;
        request.audio_codec = audio_codec;
        request.subtitle_stream_index = subtitle_stream_index;
        request.subtitle_codec = subtitle_codec;
        request.start_position_ms = timeline_base_ms_;
        request.callback_user_data = this;
        request.audio_decoder_open = &LocalMediaContinuousPlayback::open_audio_decoder_callback;
        request.audio_packet_ready = &LocalMediaContinuousPlayback::audio_packet_callback;
        request.subtitle_packet_ready = &LocalMediaContinuousPlayback::subtitle_packet_callback;

        if (subtitle_track_id != 0 && subtitle_stream_index >= 0 && !subtitle_codec.empty()) {
            subtitle_frame_bridge_.attach(subtitle_track_id, false);
        }

        const bool started = video_runtime_.start(request);
        copy_video_runtime_stats();
        copy_video_frame_buffer();
        copy_audio_runtime_stats(audio_runtime_, stats_);

        if (!started || !stats_.first_frame_decoded) {
            if (stats_.error.empty()) {
                stats_.error = "p5 local video runtime failed to start";
            }
            stats_.active = false;
            video_runtime_.close();
            audio_runtime_.close();
            return false;
        }

        stats_.active = true;
        if (frame_buffer_.valid && frame_buffer_.media_time_us > 0) {
            timeline_base_ms_ = frame_buffer_.media_time_us / 1000;
            waiting_for_seek_frame_ = false;
            seek_preroll_anchor_valid_ = false;
            stats_.p7_seek_preroll_active = false;
        }
        timeline_anchor_ = std::chrono::steady_clock::now();
        stats_.position_ms = timeline_base_ms_;
        stats_.playback_elapsed_ms = timeline_base_ms_;
    }

    start_background_pump();
    return true;
}

}  // namespace kivo::video::playback_graph::local_media
