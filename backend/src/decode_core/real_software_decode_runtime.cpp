#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include <utility>

#ifdef KIVO_ENABLE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}
#endif

namespace kivo::cinema_engine {

RealSoftwareDecodeRuntime::RealSoftwareDecodeRuntime()
    : codec_context_(nullptr),
      is_open_(false),
      is_flushed_(false),
      width_(0),
      height_(0) {}

RealSoftwareDecodeRuntime::~RealSoftwareDecodeRuntime() {
    close();
}

RealSoftwareDecodeRuntime::RealSoftwareDecodeRuntime(RealSoftwareDecodeRuntime&& other) noexcept
    : codec_context_(other.codec_context_),
      codec_name_(std::move(other.codec_name_)),
      is_open_(other.is_open_),
      is_flushed_(other.is_flushed_),
      width_(other.width_),
      height_(other.height_),
      pixel_format_(std::move(other.pixel_format_)),
      error_code_(std::move(other.error_code_)),
      inspector_hint_(std::move(other.inspector_hint_)),
      trace_id_(std::move(other.trace_id_)) {
    other.codec_context_ = nullptr;
    other.is_open_ = false;
}

RealSoftwareDecodeRuntime& RealSoftwareDecodeRuntime::operator=(RealSoftwareDecodeRuntime&& other) noexcept {
    if (this != &other) {
        close();
        codec_context_ = other.codec_context_;
        codec_name_ = std::move(other.codec_name_);
        is_open_ = other.is_open_;
        is_flushed_ = other.is_flushed_;
        width_ = other.width_;
        height_ = other.height_;
        pixel_format_ = std::move(other.pixel_format_);
        error_code_ = std::move(other.error_code_);
        inspector_hint_ = std::move(other.inspector_hint_);
        trace_id_ = std::move(other.trace_id_);
        other.codec_context_ = nullptr;
        other.is_open_ = false;
    }
    return *this;
}

bool RealSoftwareDecodeRuntime::open(const std::string& codec_name, const std::string& trace_id) {
    close();
    codec_name_ = codec_name;
    trace_id_ = trace_id;

#ifdef KIVO_ENABLE_FFMPEG
    const AVCodec* codec = avcodec_find_decoder_by_name(codec_name.c_str());
    if (!codec) {
        error_code_ = "codec_not_found";
        inspector_hint_ = "Decoder not found: " + codec_name;
        return false;
    }

    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        error_code_ = "context_alloc_failed";
        inspector_hint_ = "Failed to allocate codec context for: " + codec_name;
        return false;
    }

    int ret = avcodec_open2(ctx, codec, nullptr);
    if (ret < 0) {
        avcodec_free_context(&ctx);
        error_code_ = FfmpegAdapter::map_error(ret);
        inspector_hint_ = "Failed to open codec: " + codec_name;
        return false;
    }

    codec_context_ = ctx;
    is_open_ = true;
    is_flushed_ = false;

    width_ = ctx->width;
    height_ = ctx->height;
    if (ctx->pix_fmt != AV_PIX_FMT_NONE) {
        pixel_format_ = FfmpegAdapter::get_pixel_format_name(ctx->pix_fmt);
    }

    error_code_.clear();
    inspector_hint_ = "Decoder opened: " + codec_name;
    return true;
#else
    error_code_ = "ffmpeg_not_available";
    inspector_hint_ = "FFmpeg adapter disabled at build time";
    return false;
#endif
}

DecodeResult RealSoftwareDecodeRuntime::decode(const KivoPacket& packet, const std::string& trace_id) {
    DecodeResult result;
    result.metadata.trace_id = trace_id;

    if (!is_open_ || !codec_context_) {
        result.success = false;
        result.error_message = "not_open: Decoder is not open";
        return result;
    }

#ifdef KIVO_ENABLE_FFMPEG
    auto* ctx = static_cast<AVCodecContext*>(codec_context_);

    AVPacket* av_packet = av_packet_alloc();
    if (!av_packet) {
        result.success = false;
        result.error_message = "packet_alloc_failed: Failed to allocate packet for decoding";
        return result;
    }

    av_packet->stream_index = packet.stream_index;
    av_packet->pts = packet.pts;
    av_packet->dts = packet.dts;
    av_packet->duration = static_cast<int>(packet.duration);
    if (packet.is_key_frame) {
        av_packet->flags |= AV_PKT_FLAG_KEY;
    }

    int ret = avcodec_send_packet(ctx, av_packet);
    av_packet_free(&av_packet);

    if (ret < 0) {
        if (ret == AVERROR(EAGAIN)) {
            result.needs_more_input = true;
            result.success = true;
        } else if (ret == AVERROR_EOF) {
            result.eof = true;
            result.success = true;
        } else {
            result.success = false;
            result.error_message = FfmpegAdapter::map_error(ret);
        }
        return result;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        result.success = false;
        result.error_message = "frame_alloc_failed: Failed to allocate frame";
        return result;
    }

    ret = avcodec_receive_frame(ctx, frame);
    if (ret < 0) {
        av_frame_free(&frame);
        if (ret == AVERROR(EAGAIN)) {
            result.needs_more_input = true;
            result.success = true;
        } else if (ret == AVERROR_EOF) {
            result.eof = true;
            result.success = true;
        } else {
            result.success = false;
            result.error_message = FfmpegAdapter::map_error(ret);
        }
        return result;
    }

    result.frame.frame_id = "frame_" + std::to_string(frame->pts);
    result.frame.pts = frame->pts;
    result.frame.dts = frame->dts;
    result.frame.duration = frame->duration;

    if (frame->width > 0 && frame->height > 0) {
        result.frame.stream_kind = "video";
        result.frame.width = frame->width;
        result.frame.height = frame->height;
        if (frame->format != AV_PIX_FMT_NONE) {
            result.frame.pixel_format = FfmpegAdapter::get_pixel_format_name(frame->format);
            pixel_format_ = result.frame.pixel_format;
        }
        width_ = frame->width;
        height_ = frame->height;
    } else if (frame->nb_samples > 0) {
        result.frame.stream_kind = "audio";
        result.frame.sample_rate = frame->sample_rate;
        result.frame.channels = frame->ch_layout.nb_channels;
        result.frame.sample_format = FfmpegAdapter::get_sample_format_name(frame->format);
    }

    av_frame_free(&frame);
    result.success = true;
    return result;
#else
    result.success = false;
    result.error_message = "ffmpeg_not_available: FFmpeg adapter disabled at build time";
    return result;
#endif
}

DecodeResult RealSoftwareDecodeRuntime::flush(const std::string& trace_id) {
    DecodeResult result;
    result.metadata.trace_id = trace_id;

    if (!is_open_ || !codec_context_) {
        result.success = false;
        result.error_message = "not_open: Decoder is not open";
        return result;
    }

#ifdef KIVO_ENABLE_FFMPEG
    auto* ctx = static_cast<AVCodecContext*>(codec_context_);

    int ret = avcodec_send_packet(ctx, nullptr);
    if (ret < 0 && ret != AVERROR_EOF) {
        result.success = false;
        result.error_message = FfmpegAdapter::map_error(ret);
        return result;
    }

    AVFrame* frame = av_frame_alloc();
    while (true) {
        ret = avcodec_receive_frame(ctx, frame);
        if (ret < 0) break;
    }
    av_frame_free(&frame);

    is_flushed_ = true;
    result.success = true;
    result.eof = true;
    return result;
#else
    result.success = false;
    result.error_message = "ffmpeg_not_available: FFmpeg adapter disabled at build time";
    return result;
#endif
}

void RealSoftwareDecodeRuntime::close() {
#ifdef KIVO_ENABLE_FFMPEG
    if (codec_context_) {
        FfmpegAdapter::free_codec_context(static_cast<AVCodecContext*>(codec_context_));
        codec_context_ = nullptr;
    }
#endif
    is_open_ = false;
    is_flushed_ = false;
    width_ = 0;
    height_ = 0;
}

bool RealSoftwareDecodeRuntime::is_open() const { return is_open_; }
bool RealSoftwareDecodeRuntime::is_flushed() const { return is_flushed_; }
std::string RealSoftwareDecodeRuntime::codec_name() const { return codec_name_; }
int32_t RealSoftwareDecodeRuntime::width() const { return width_; }
int32_t RealSoftwareDecodeRuntime::height() const { return height_; }
std::string RealSoftwareDecodeRuntime::pixel_format() const { return pixel_format_; }
std::string RealSoftwareDecodeRuntime::error_code() const { return error_code_; }
std::string RealSoftwareDecodeRuntime::inspector_hint() const { return inspector_hint_; }

}  // namespace kivo::cinema_engine
