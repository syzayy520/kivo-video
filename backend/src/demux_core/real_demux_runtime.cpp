#include "kivo/cinema_engine/demux_core/real_demux_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include <utility>

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

    FfmpegFormatHandle handle = FfmpegAdapter::open_file(path);
    if (!handle.native) {
        if (!FfmpegAdapter::is_available()) {
            error_code_ = "ffmpeg_not_available";
            inspector_hint_ = "FFmpeg adapter disabled at build time";
        } else {
            error_code_ = "open_failed";
            inspector_hint_ = "Failed to open file for demuxing: " + path;
        }
        return false;
    }

    format_context_ = handle.native;

    std::string container_format;
    double dur = 0.0;
    int64_t bitrate = 0;
    FfmpegAdapter::probe_container(handle, container_format, duration_, bitrate);
    container_format_ = container_format;

    std::vector<int> video_idx, audio_idx, subtitle_idx;
    FfmpegAdapter::enumerate_streams(handle, video_idx, audio_idx, subtitle_idx);
    video_stream_count_ = static_cast<int>(video_idx.size());
    audio_stream_count_ = static_cast<int>(audio_idx.size());
    subtitle_stream_count_ = static_cast<int>(subtitle_idx.size());

    is_open_ = true;
    is_eof_ = false;
    error_code_.clear();
    inspector_hint_ = "Demuxer opened: " + path + " [" + container_format_ + "]";
    return true;
}

void RealDemuxRuntime::close() {
    if (format_context_) {
        FfmpegFormatHandle handle;
        handle.native = format_context_;
        FfmpegAdapter::free_format_context(handle);
        format_context_ = nullptr;
    }
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

    FfmpegFormatHandle handle;
    handle.native = format_context_;

    FfmpegReadResult read_result = FfmpegAdapter::read_packet(handle);

    if (read_result.eof) {
        is_eof_ = true;
        result.success = true;
        result.eof = true;
        return result;
    }

    if (!read_result.success) {
        result.success = false;
        result.error_message = read_result.error_message;
        return result;
    }

    result.packet.stream_index = read_result.packet.stream_index;
    result.packet.pts = read_result.packet.pts;
    result.packet.dts = read_result.packet.dts;
    result.packet.duration = read_result.packet.duration;
    result.packet.is_key_frame = read_result.packet.is_key_frame;
    result.packet.data = std::move(read_result.packet_data);
    result.packet.packet_id = "pkt_" + std::to_string(read_result.packet.stream_index) + "_" +
                              std::to_string(read_result.packet.pts);

    result.success = true;
    return result;
}

bool RealDemuxRuntime::seek_to_time(double seconds) {
    if (!is_open_ || !format_context_) {
        return false;
    }

    FfmpegFormatHandle handle;
    handle.native = format_context_;

    if (!FfmpegAdapter::seek(handle, seconds)) {
        error_code_ = "seek_failed";
        inspector_hint_ = "Seek to " + std::to_string(seconds) + "s failed";
        return false;
    }

    is_eof_ = false;
    error_code_.clear();
    inspector_hint_ = "Seek to " + std::to_string(seconds) + "s OK";
    return true;
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
