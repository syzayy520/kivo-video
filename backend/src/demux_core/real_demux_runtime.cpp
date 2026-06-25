#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include <utility>

#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#endif

namespace kivo::cinema_engine {

RealDemuxRuntime::RealDemuxRuntime()
    : format_context_(nullptr),
      duration_(0.0),
      is_open_(false),
      is_eof_(false),
      video_stream_count_(0),
      audio_stream_count_(0),
      subtitle_stream_count_(0) {}

RealDemuxRuntime::~RealDemuxRuntime() {
    close();
}

RealDemuxRuntime::RealDemuxRuntime(RealDemuxRuntime&& other) noexcept
    : format_context_(other.format_context_),
      container_format_(std::move(other.container_format_)),
      duration_(other.duration_),
      is_open_(other.is_open_),
      is_eof_(other.is_eof_),
      error_code_(std::move(other.error_code_)),
      inspector_hint_(std::move(other.inspector_hint_)),
      trace_id_(std::move(other.trace_id_)),
      video_stream_count_(other.video_stream_count_),
      audio_stream_count_(other.audio_stream_count_),
      subtitle_stream_count_(other.subtitle_stream_count_) {
    other.format_context_ = nullptr;
    other.is_open_ = false;
}

RealDemuxRuntime& RealDemuxRuntime::operator=(RealDemuxRuntime&& other) noexcept {
    if (this != &other) {
        close();
        format_context_ = other.format_context_;
        container_format_ = std::move(other.container_format_);
        duration_ = other.duration_;
        is_open_ = other.is_open_;
        is_eof_ = other.is_eof_;
        error_code_ = std::move(other.error_code_);
        inspector_hint_ = std::move(other.inspector_hint_);
        trace_id_ = std::move(other.trace_id_);
        video_stream_count_ = other.video_stream_count_;
        audio_stream_count_ = other.audio_stream_count_;
        subtitle_stream_count_ = other.subtitle_stream_count_;
        other.format_context_ = nullptr;
        other.is_open_ = false;
    }
    return *this;
}

bool RealDemuxRuntime::open(const std::string& path, const std::string& trace_id) {
    close();
    trace_id_ = trace_id;

#ifdef KIVO_ENABLE_FFMPEG
    auto* ctx = FfmpegAdapter::open_file(path);
    if (!ctx) {
        error_code_ = "open_failed";
        inspector_hint_ = "Failed to open file for demuxing: " + path;
        return false;
    }

    format_context_ = ctx;

    std::string container_format;
    double duration = 0.0;
    int64_t bitrate = 0;
    FfmpegAdapter::probe_container(ctx, container_format, duration, bitrate);
    container_format_ = container_format;
    duration_ = duration;

    std::vector<int> video_idx, audio_idx, subtitle_idx;
    FfmpegAdapter::enumerate_streams(ctx, video_idx, audio_idx, subtitle_idx);
    video_stream_count_ = static_cast<int>(video_idx.size());
    audio_stream_count_ = static_cast<int>(audio_idx.size());
    subtitle_stream_count_ = static_cast<int>(subtitle_idx.size());

    is_open_ = true;
    is_eof_ = false;
    error_code_.clear();
    inspector_hint_ = "Demuxer opened: " + path + " [" + container_format_ + "]";
    return true;
#else
    error_code_ = "ffmpeg_not_available";
    inspector_hint_ = "FFmpeg adapter disabled at build time";
    return false;
#endif
}

void RealDemuxRuntime::close() {
#ifdef KIVO_ENABLE_FFMPEG
    if (format_context_) {
        FfmpegAdapter::free_format_context(static_cast<AVFormatContext*>(format_context_));
        format_context_ = nullptr;
    }
#endif
    is_open_ = false;
    is_eof_ = false;
    video_stream_count_ = 0;
    audio_stream_count_ = 0;
    subtitle_stream_count_ = 0;
}

DemuxResult RealDemuxRuntime::read_packet(const std::string& trace_id) {
    DemuxResult result;
    result.metadata.trace_id = trace_id;

    if (!is_open_ || !format_context_) {
        result.success = false;
        result.error_message = "not_open: Demuxer is not open";
        return result;
    }

#ifdef KIVO_ENABLE_FFMPEG
    auto* ctx = static_cast<AVFormatContext*>(format_context_);
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        result.success = false;
        result.error_message = "packet_alloc_failed: Failed to allocate packet";
        return result;
    }

    int ret = av_read_frame(ctx, packet);
    if (ret < 0) {
        av_packet_free(&packet);
        if (ret == AVERROR_EOF || avio_feof(ctx->pb)) {
            is_eof_ = true;
            result.success = true;
            result.eof = true;
        } else {
            result.success = false;
            result.error_message = FfmpegAdapter::map_error(ret);
        }
        return result;
    }

    result.packet.stream_index = packet->stream_index;
    result.packet.pts = (packet->pts != AV_NOPTS_VALUE) ? packet->pts : 0;
    result.packet.dts = (packet->dts != AV_NOPTS_VALUE) ? packet->dts : 0;
    result.packet.duration = packet->duration;
    result.packet.is_key_frame = (packet->flags & AV_PKT_FLAG_KEY) != 0;
    result.packet.data_size = packet->size;
    result.packet.packet_id = "pkt_" + std::to_string(packet->stream_index) + "_" +
                              std::to_string(packet->pts);

    av_packet_free(&packet);
    result.success = true;
    return result;
#else
    result.success = false;
    result.error_message = "ffmpeg_not_available: FFmpeg adapter disabled at build time";
    return result;
#endif
}

bool RealDemuxRuntime::seek_to_time(double seconds) {
    if (!is_open_ || !format_context_) {
        return false;
    }

#ifdef KIVO_ENABLE_FFMPEG
    auto* ctx = static_cast<AVFormatContext*>(format_context_);
    int64_t target_ts = static_cast<int64_t>(seconds * AV_TIME_BASE);
    int ret = av_seek_frame(ctx, -1, target_ts, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        error_code_ = FfmpegAdapter::map_error(ret);
        inspector_hint_ = "Seek failed";
        return false;
    }
    avcodec_flush_buffers(ctx->streams[0]->codecpar ? nullptr : nullptr);
    is_eof_ = false;
    error_code_.clear();
    inspector_hint_ = "Seek to " + std::to_string(seconds) + "s OK";
    return true;
#else
    return false;
#endif
}

bool RealDemuxRuntime::is_open() const { return is_open_; }
bool RealDemuxRuntime::is_eof() const { return is_eof_; }
std::string RealDemuxRuntime::container_format() const { return container_format_; }
double RealDemuxRuntime::duration() const { return duration_; }
std::string RealDemuxRuntime::error_code() const { return error_code_; }
std::string RealDemuxRuntime::inspector_hint() const { return inspector_hint_; }
int RealDemuxRuntime::video_stream_count() const { return video_stream_count_; }
int RealDemuxRuntime::audio_stream_count() const { return audio_stream_count_; }
int RealDemuxRuntime::subtitle_stream_count() const { return subtitle_stream_count_; }

}  // namespace kivo::cinema_engine
