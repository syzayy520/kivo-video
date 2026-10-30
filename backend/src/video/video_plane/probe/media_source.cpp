#include "video/video_plane/probe/media_source.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
}

#include <utility>

namespace kivo::video::video_plane::probe {
namespace {

[[nodiscard]] auto ff_error_text(const int code) -> std::string {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(code, buffer, sizeof(buffer)) < 0) {
        return "error " + std::to_string(code);
    }
    return buffer;
}

}  // namespace

MediaSource::~MediaSource() {
    close();
}

auto MediaSource::open(const std::string& path, std::string& error) -> bool {
    close();

    AVFormatContext* context = nullptr;
    const int open_result = avformat_open_input(&context, path.c_str(), nullptr, nullptr);
    if (open_result < 0) {
        error = "avformat_open_input failed: " + ff_error_text(open_result);
        return false;
    }
    format_ctx_ = context;

    const int info_result = avformat_find_stream_info(format_ctx_, nullptr);
    if (info_result < 0) {
        error = "avformat_find_stream_info failed: " + ff_error_text(info_result);
        close();
        return false;
    }

    const int index = av_find_best_stream(format_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0) {
        error = "no video stream in container";
        close();
        return false;
    }
    video_stream_index_ = index;

    const AVStream* stream = format_ctx_->streams[index];
    const AVCodecParameters* params = stream->codecpar;

    video_info_.codec_name = avcodec_get_name(params->codec_id);
    video_info_.width = params->width;
    video_info_.height = params->height;
    video_info_.time_base_num = stream->time_base.num;
    video_info_.time_base_den = stream->time_base.den;
    video_info_.extradata = params->extradata;
    video_info_.extradata_size = params->extradata_size > 0
                                     ? static_cast<std::size_t>(params->extradata_size)
                                     : 0;

    if (const AVPixFmtDescriptor* descriptor =
            av_pix_fmt_desc_get(static_cast<AVPixelFormat>(params->format));
        descriptor != nullptr) {
        video_info_.bit_depth = descriptor->comp[0].depth;
        video_info_.container_pixel_format = descriptor->name;
    } else {
        video_info_.container_pixel_format = "unknown";
    }

    if (stream->duration != AV_NOPTS_VALUE) {
        video_info_.duration_us =
            av_rescale_q(stream->duration, stream->time_base, AV_TIME_BASE_Q);
    }

    packet_ = av_packet_alloc();
    if (packet_ == nullptr) {
        error = "av_packet_alloc failed";
        close();
        return false;
    }
    return true;
}

void MediaSource::close() noexcept {
    if (packet_ != nullptr) {
        av_packet_free(&packet_);
    }
    if (format_ctx_ != nullptr) {
        avformat_close_input(&format_ctx_);
    }
    video_stream_index_ = -1;
    packet_view_ = SourcePacketView{};
}

auto MediaSource::read_next(std::string& error) -> ReadStatus {
    if (format_ctx_ == nullptr || packet_ == nullptr || video_stream_index_ < 0) {
        error = "media source is not open";
        return ReadStatus::Failed;
    }

    for (;;) {
        av_packet_unref(packet_);
        const int read_result = av_read_frame(format_ctx_, packet_);
        if (read_result == AVERROR_EOF) {
            return ReadStatus::EndOfStream;
        }
        if (read_result < 0) {
            error = "av_read_frame failed: " + ff_error_text(read_result);
            return ReadStatus::Failed;
        }
        if (packet_->stream_index != video_stream_index_) {
            continue;  // audio and subtitle packets are irrelevant to a video probe
        }

        packet_view_.data = packet_->data;
        packet_view_.size = static_cast<std::size_t>(packet_->size);
        packet_view_.pts = packet_->pts;
        packet_view_.dts = packet_->dts;
        packet_view_.key_frame = (packet_->flags & AV_PKT_FLAG_KEY) != 0;
        return ReadStatus::Packet;
    }
}

}  // namespace kivo::video::video_plane::probe
