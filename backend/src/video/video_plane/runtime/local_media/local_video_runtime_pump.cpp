#include "video/video_plane/runtime/local_media/local_video_runtime.hpp"

#include <algorithm>
#include <memory>
#include <utility>

#include "video/video_plane/runtime/local_media/local_video_ffmpeg_session.hpp"
#include "video/video_plane/runtime/local_media/local_video_rgba_converter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/demux_core/kivo_packet.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#endif

namespace kivo::video::video_plane::runtime::local_media {
namespace {

[[nodiscard]] std::uint64_t rgba_signature(const std::vector<std::uint8_t>& rgba) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    const std::size_t sample_bytes = std::min<std::size_t>(rgba.size(), 4096);
    for (std::size_t index = 0; index < sample_bytes; ++index) {
        hash ^= static_cast<std::uint64_t>(rgba[index]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

#ifdef KIVO_ENABLE_FFMPEG
[[nodiscard]] std::int64_t packet_time_us(
    const kivo::cinema_engine::FfmpegFormatHandle handle,
    const std::int32_t stream_index,
    const std::int64_t timestamp,
    const std::int64_t fallback_us) noexcept {
    std::int64_t microseconds = fallback_us;
    if (!kivo::cinema_engine::FfmpegAdapter::rescale_stream_timestamp_us(
            handle, stream_index, timestamp, microseconds)) {
        return fallback_us;
    }
    return microseconds;
}

[[nodiscard]] std::int64_t packet_start_time_us(
    const kivo::cinema_engine::FfmpegFormatHandle handle,
    const std::int32_t stream_index,
    const kivo::cinema_engine::FfmpegReadResult& read_result) noexcept {
    const std::int64_t timestamp =
        read_result.packet.pts != 0 ? read_result.packet.pts : read_result.packet.dts;
    return packet_time_us(handle, stream_index, timestamp, 0);
}

[[nodiscard]] std::int64_t packet_duration_us(
    const kivo::cinema_engine::FfmpegFormatHandle handle,
    const std::int32_t stream_index,
    const kivo::cinema_engine::FfmpegReadResult& read_result) noexcept {
    if (read_result.packet.duration <= 0) {
        return 0;
    }
    return packet_time_us(handle, stream_index, read_result.packet.duration, 0);
}

[[nodiscard]] bool decode_video_frame(
    kivo::cinema_engine::DecodeResult&& result,
    const std::int64_t frame_time_us,
    const std::int64_t frame_duration_us,
    LocalVideoFrameBuffer& frame_buffer,
    LocalVideoRuntimeStats& stats,
    LocalVideoD3d11Upload& d3d11_upload) noexcept {
    const bool d3d11_uploaded = d3d11_upload.upload(result.frame);
    (void)d3d11_uploaded;
    const auto& upload_stats = d3d11_upload.stats();
    stats.d3d11_upload_available = upload_stats.available;
    stats.d3d11_upload_initialized = upload_stats.initialized;
    stats.d3d11_uploaded = upload_stats.uploaded;
    stats.d3d11_frames_uploaded = upload_stats.frames_uploaded;
    stats.d3d11_upload_failures = upload_stats.upload_failures;
    stats.d3d11_upload_last_elapsed_ms = upload_stats.last_upload_elapsed_ms;
    stats.d3d11_upload_max_elapsed_ms = upload_stats.max_upload_elapsed_ms;
    stats.d3d11_device_type = upload_stats.device_type;
    stats.d3d11_upload_error = upload_stats.error;

    std::vector<std::uint8_t> rgba_pixels;
    if (result.frame.pixel_format == "rgba") {
        const auto expected_size =
            static_cast<std::size_t>(result.frame.width) *
            static_cast<std::size_t>(result.frame.height) * 4U;
        if (result.frame.frame_data.size() < expected_size) {
            stats.error = "rgba frame is truncated";
            return false;
        }
        rgba_pixels = std::move(result.frame.frame_data);
    } else {
        std::string error;
        if (!convert_decoded_frame_to_rgba8888_into(result.frame, rgba_pixels, error)) {
            stats.error = error.empty() ? "rgba conversion failed" : error;
            return false;
        }
    }

    frame_buffer.valid = true;
    frame_buffer.width = result.frame.width;
    frame_buffer.height = result.frame.height;
    frame_buffer.media_time_us = frame_time_us;
    frame_buffer.duration_us = frame_duration_us;
    frame_buffer.rgba8888 =
        std::make_shared<const std::vector<std::uint8_t>>(std::move(rgba_pixels));

    ++stats.decoded_frame_count;
    ++stats.rendered_frame_count;
    stats.first_frame_decoded = true;
    stats.frame_rendered = true;
    stats.last_frame_signature = rgba_signature(*frame_buffer.rgba8888);
    stats.last_frame_time_us = frame_time_us;
    stats.last_frame_duration_us = frame_duration_us;
    return true;
}
#endif

}  // namespace

int LocalVideoRuntime::pump_packets(const int max_packets) noexcept {
#ifndef KIVO_ENABLE_FFMPEG
    (void)max_packets;
    return 0;
#else
    if (!stats_.active || !stats_.demux_started || ffmpeg_session_ == nullptr) {
        return 0;
    }

    using namespace kivo::cinema_engine;
    int new_frames = 0;
    for (int packet_budget = 0; packet_budget < max_packets; ++packet_budget) {
        const auto read_result = FfmpegAdapter::read_packet(ffmpeg_session_->format_handle);
        if (read_result.eof) {
            break;
        }
        if (!read_result.success) {
            continue;
        }
        if (read_result.packet.stream_index == audio_stream_index_) {
            if (discard_before_media_time_us_ >= 0) {
                const std::int64_t packet_start_us =
                    packet_start_time_us(ffmpeg_session_->format_handle,
                                         read_result.packet.stream_index,
                                         read_result);
                if (packet_start_us < discard_before_media_time_us_) {
                    continue;
                }
            }
            LocalVideoAudioPacket audio_packet;
            audio_packet.stream_index = read_result.packet.stream_index;
            audio_packet.pts = read_result.packet.pts;
            audio_packet.dts = read_result.packet.dts;
            audio_packet.duration = read_result.packet.duration;
            audio_packet.is_key_frame = read_result.packet.is_key_frame;
            audio_packet.data = std::move(read_result.packet_data);
            for (auto& side : read_result.side_data) {
                audio_packet.side_data.push_back(
                    {side.type, std::move(side.data)});
            }
            dispatch_audio_packet(std::move(audio_packet));
            continue;
        }
        if (read_result.packet.stream_index == subtitle_stream_index_) {
            if (discard_before_media_time_us_ >= 0) {
                const std::int64_t packet_end_us =
                    packet_start_time_us(ffmpeg_session_->format_handle,
                                         read_result.packet.stream_index,
                                         read_result) +
                    packet_duration_us(ffmpeg_session_->format_handle,
                                       read_result.packet.stream_index,
                                       read_result);
                if (packet_end_us < discard_before_media_time_us_) {
                    continue;
                }
            }
            LocalVideoSubtitlePacket subtitle_packet;
            subtitle_packet.stream_index = read_result.packet.stream_index;
            subtitle_packet.codec_name = subtitle_codec_;
            subtitle_packet.is_key_frame = read_result.packet.is_key_frame;
            subtitle_packet.start_time_us =
                packet_time_us(ffmpeg_session_->format_handle,
                               read_result.packet.stream_index,
                               read_result.packet.pts,
                               0);
            const std::int64_t duration_us =
                read_result.packet.duration > 0
                    ? packet_time_us(ffmpeg_session_->format_handle,
                                     read_result.packet.stream_index,
                                     read_result.packet.duration,
                                     5'000'000)
                    : 5'000'000;
            subtitle_packet.end_time_us = subtitle_packet.start_time_us + duration_us;
            subtitle_packet.data = std::move(read_result.packet_data);
            dispatch_subtitle_packet(std::move(subtitle_packet));
            continue;
        }
        if (read_result.packet.stream_index != video_stream_index_) {
            continue;
        }

        KivoPacket packet{};
        packet.stream_index = read_result.packet.stream_index;
        packet.pts = read_result.packet.pts;
        packet.dts = read_result.packet.dts;
        packet.duration = read_result.packet.duration;
        packet.is_key_frame = read_result.packet.is_key_frame;
        packet.data = std::move(read_result.packet_data);
        for (auto& side : read_result.side_data) {
            packet.side_data.push_back({side.type, std::move(side.data)});
        }
        packet.packet_id = "pkt_" + std::to_string(packet.stream_index) + "_" +
                           std::to_string(packet.pts);
        ++next_packet_index_;

        DecodeResult result = ffmpeg_session_->decoder.decode_video_rgba(
            packet, "p5_local_video_decode_" + std::to_string(next_packet_index_));
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 ||
            result.frame.frame_data.empty()) {
            continue;
        }
        const std::int64_t frame_time_us =
            packet_time_us(ffmpeg_session_->format_handle,
                           read_result.packet.stream_index,
                           result.frame.pts != 0 ? result.frame.pts : read_result.packet.pts,
                           packet_time_us(ffmpeg_session_->format_handle,
                                          read_result.packet.stream_index,
                                          read_result.packet.pts,
                                          0));
        const std::int64_t frame_duration_us =
            result.frame.duration > 0
                ? packet_time_us(ffmpeg_session_->format_handle,
                                 read_result.packet.stream_index,
                                 result.frame.duration,
                                 0)
                : (read_result.packet.duration > 0
                       ? packet_time_us(ffmpeg_session_->format_handle,
                                        read_result.packet.stream_index,
                                        read_result.packet.duration,
                                        0)
                       : 0);
        if (discard_before_media_time_us_ >= 0 &&
            frame_time_us < discard_before_media_time_us_) {
            continue;
        }
        if (!decode_video_frame(std::move(result),
                                frame_time_us,
                                frame_duration_us,
                                frame_buffer_,
                                stats_,
                                d3d11_upload_)) {
            continue;
        }
        discard_before_media_time_us_ = -1;
        ++new_frames;
    }
    return new_frames;
#endif
}

void LocalVideoRuntime::dispatch_audio_packet(LocalVideoAudioPacket&& packet) noexcept {
    if (audio_packet_ready_ == nullptr) {
        return;
    }
    ++stats_.audio_packets_dispatched;
    audio_packet_ready_(callback_user_data_, std::move(packet));
}

void LocalVideoRuntime::dispatch_subtitle_packet(LocalVideoSubtitlePacket&& packet) noexcept {
    if (subtitle_packet_ready_ == nullptr) {
        return;
    }
    ++stats_.subtitle_packets_dispatched;
    subtitle_packet_ready_(callback_user_data_, std::move(packet));
}

}  // namespace kivo::video::video_plane::runtime::local_media
