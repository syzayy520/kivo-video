#include "video/playback_graph/local_media/local_media_continuous_playback.hpp"

#include <algorithm>

#include "video/playback_graph/local_media/local_media_frame_rgba_converter.hpp"

#ifdef KIVO_ENABLE_FFMPEG
#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"
#endif

namespace kivo::video::playback_graph::local_media {
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
[[nodiscard]] bool decode_video_frame(
    const kivo::cinema_engine::DecodeResult& result,
    LocalMediaFrameBuffer& frame_buffer,
    LocalMediaContinuousStats& stats) noexcept {
    const auto rgba = convert_decoded_frame_to_rgba8888(result.frame);
    if (!rgba.valid) {
        stats.error = rgba.error.empty() ? "rgba conversion failed" : rgba.error;
        return false;
    }

    frame_buffer.rgba8888 = rgba.rgba8888;
    frame_buffer.valid = true;
    frame_buffer.width = rgba.width;
    frame_buffer.height = rgba.height;

    ++stats.decoded_frame_count;
    ++stats.rendered_frame_count;
    stats.first_frame_decoded = true;
    stats.frame_rendered = true;
    stats.last_frame_signature = rgba_signature(frame_buffer.rgba8888);
    return true;
}
#endif

}  // namespace

#ifdef KIVO_ENABLE_FFMPEG
struct LocalMediaContinuousPlayback::FfmpegSession {
    void* format_handle{nullptr};
    void* codec_handle{nullptr};
    kivo::cinema_engine::RealDemuxRuntime demux{};
    kivo::cinema_engine::RealSoftwareDecodeRuntime decoder{};
};
#else
struct LocalMediaContinuousPlayback::FfmpegSession {};
#endif

LocalMediaContinuousPlayback::LocalMediaContinuousPlayback() = default;

LocalMediaContinuousPlayback::~LocalMediaContinuousPlayback() {
    close();
}

bool LocalMediaContinuousPlayback::start(
    const std::string& media_path,
    const int video_stream_index,
    const std::string& video_codec) noexcept {
    close();
    stats_ = {};
    stats_.active = true;
    media_path_ = media_path;
    video_stream_index_ = video_stream_index;
    video_codec_ = video_codec;
    playback_started_ = std::chrono::steady_clock::now();

#ifndef KIVO_ENABLE_FFMPEG
    stats_.error = "ffmpeg disabled";
    stats_.active = false;
    return false;
#else
    ffmpeg_session_ = std::make_unique<FfmpegSession>();
    using namespace kivo::cinema_engine;

    FfmpegFormatHandle fmt_handle = FfmpegAdapter::open_file(media_path_);
    if (!fmt_handle.native) {
        stats_.error = "format open failed";
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }
    ffmpeg_session_->format_handle = fmt_handle.native;

    FfmpegCodecHandle codec_handle =
        FfmpegAdapter::create_codec_context(fmt_handle, video_stream_index_);
    if (!codec_handle.native) {
        FfmpegAdapter::free_format_context(fmt_handle);
        stats_.error = "codec context failed";
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }
    ffmpeg_session_->codec_handle = codec_handle.native;

    if (!ffmpeg_session_->decoder.adopt_codec_context(
            codec_handle.native, video_codec_, "p10_continuous_decode")) {
        FfmpegAdapter::free_format_context(fmt_handle);
        stats_.error = "codec adopt failed";
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }

    if (!ffmpeg_session_->demux.open(media_path_, "p10_continuous_demux")) {
        ffmpeg_session_->decoder.close();
        FfmpegAdapter::free_format_context(fmt_handle);
        stats_.error = "demux open failed";
        stats_.active = false;
        ffmpeg_session_.reset();
        return false;
    }

    stats_.demux_started = true;
    stats_.video_decode_started = true;
    stats_.continuous_rendering = true;
    next_packet_index_ = 0;

    if (pump_packets(5000) <= 0 || !stats_.first_frame_decoded) {
        if (stats_.error.empty()) {
            stats_.error = "no video frame decoded";
        }
        close();
        return false;
    }
    return true;
#endif
}

int LocalMediaContinuousPlayback::pump_packets(const int max_packets) noexcept {
#ifndef KIVO_ENABLE_FFMPEG
    return 0;
#else
    if (!stats_.active || !stats_.demux_started || ffmpeg_session_ == nullptr) {
        return 0;
    }

    using namespace kivo::cinema_engine;
    int new_frames = 0;
    for (int packet_budget = 0; packet_budget < max_packets; ++packet_budget) {
        const auto packet =
            ffmpeg_session_->demux.read_packet("p10_continuous_packet_" + std::to_string(next_packet_index_++));
        if (packet.eof) {
            break;
        }
        if (!packet.success || packet.packet.stream_index != video_stream_index_) {
            continue;
        }

        const DecodeResult result = ffmpeg_session_->decoder.decode(
            packet.packet, "p10_continuous_decode_" + std::to_string(next_packet_index_));
        if (!result.success || result.needs_more_input || result.eof) {
            continue;
        }
        if (result.frame.width <= 0 || result.frame.height <= 0 || result.frame.frame_data.empty()) {
            continue;
        }
        if (!decode_video_frame(result, frame_buffer_, stats_)) {
            continue;
        }
        ++new_frames;
    }

    const auto elapsed = std::chrono::steady_clock::now() - playback_started_;
    stats_.playback_elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    stats_.position_ms = stats_.playback_elapsed_ms;
    return new_frames;
#endif
}

void LocalMediaContinuousPlayback::close() noexcept {
#ifndef KIVO_ENABLE_FFMPEG
    stats_ = {};
    frame_buffer_ = {};
    media_path_.clear();
    video_stream_index_ = -1;
    video_codec_.clear();
    next_packet_index_ = 0;
    ffmpeg_session_.reset();
    return;
#else
    if (ffmpeg_session_ != nullptr) {
        using namespace kivo::cinema_engine;

        ffmpeg_session_->decoder.close();
        ffmpeg_session_->demux.close();
        if (ffmpeg_session_->format_handle != nullptr) {
            FfmpegFormatHandle fmt_handle{ffmpeg_session_->format_handle};
            FfmpegAdapter::free_format_context(fmt_handle);
        }
    }
    ffmpeg_session_.reset();

    stats_ = {};
    frame_buffer_ = {};
    media_path_.clear();
    video_stream_index_ = -1;
    video_codec_.clear();
    next_packet_index_ = 0;
#endif
}

}  // namespace kivo::video::playback_graph::local_media