#include "kivo/cinema_engine/decode_core/real_software_decode_runtime.hpp"
#include "kivo/cinema_engine/third_party_adapter/ffmpeg_adapter/ffmpeg_adapter.hpp"

#include <utility>

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

    FfmpegCodecHandle handle = FfmpegAdapter::open_decoder_by_name(codec_name);
    if (!handle.native) {
        if (!FfmpegAdapter::is_available()) {
            error_code_ = "ffmpeg_not_available";
            inspector_hint_ = "FFmpeg adapter disabled at build time";
        } else {
            error_code_ = "codec_not_found";
            inspector_hint_ = "Decoder not found: " + codec_name;
        }
        return false;
    }

    codec_context_ = handle.native;
    is_open_ = true;
    is_flushed_ = false;

    FfmpegCodecInfo info = FfmpegAdapter::get_codec_info(handle);
    width_ = info.width;
    height_ = info.height;
    pixel_format_ = info.pixel_format;

    error_code_.clear();
    inspector_hint_ = "Decoder opened: " + codec_name;
    return true;
}

bool RealSoftwareDecodeRuntime::adopt_codec_context(void* native_context,
                                                     const std::string& codec_name,
                                                     const std::string& trace_id) {
    close();
    if (!native_context) {
        error_code_ = "null_context";
        inspector_hint_ = "Cannot adopt null native context";
        return false;
    }

    codec_context_ = native_context;
    codec_name_ = codec_name;
    trace_id_ = trace_id;
    is_open_ = true;
    is_flushed_ = false;

    FfmpegCodecHandle handle{native_context};
    FfmpegCodecInfo info = FfmpegAdapter::get_codec_info(handle);
    width_ = info.width;
    height_ = info.height;
    pixel_format_ = info.pixel_format;

    error_code_.clear();
    inspector_hint_ = "Decoder adopted: " + codec_name;
    return true;
}

DecodeResult RealSoftwareDecodeRuntime::decode(const KivoPacket& packet, const std::string& trace_id) {
    DecodeResult result;
    result.metadata.trace_id = trace_id;

    if (!is_open_ || !codec_context_) {
        result.success = false;
        result.error_message = "not_open: Decoder is not open";
        return result;
    }

    FfmpegCodecHandle codec;
    codec.native = codec_context_;

    FfmpegPacketInfo pkt_info;
    pkt_info.stream_index = packet.stream_index;
    pkt_info.pts = packet.pts;
    pkt_info.dts = packet.dts;
    pkt_info.duration = static_cast<int>(packet.duration);
    pkt_info.is_key_frame = packet.is_key_frame;

    FfmpegSendResult send_result = FfmpegAdapter::send_packet(
        codec, pkt_info,
        packet.data.empty() ? nullptr : packet.data.data(),
        static_cast<int>(packet.data.size()));

    if (send_result.status == FfmpegSendStatus::NeedDrain) {
        result.needs_more_input = true;
        result.success = true;
        return result;
    }
    if (send_result.status == FfmpegSendStatus::EndOfStream) {
        result.eof = true;
        result.success = true;
        return result;
    }
    if (send_result.status == FfmpegSendStatus::Error) {
        result.success = false;
        result.error_message = send_result.error_message;
        return result;
    }

    // Try to receive a frame
    FfmpegFrameResult frame;
    if (!FfmpegAdapter::receive_frame(codec, frame)) {
        result.needs_more_input = true;
        result.success = true;
        return result;
    }

    result.frame.frame_id = "frame_" + std::to_string(frame.pts);
    result.frame.pts = frame.pts;
    result.frame.dts = frame.dts;
    result.frame.duration = frame.duration;

    if (frame.is_video) {
        result.frame.stream_kind = "video";
        result.frame.width = frame.width;
        result.frame.height = frame.height;
        result.frame.pixel_format = frame.pixel_format;
        pixel_format_ = frame.pixel_format;
        width_ = frame.width;
        height_ = frame.height;
    } else if (frame.is_audio) {
        result.frame.stream_kind = "audio";
        result.frame.sample_rate = frame.sample_rate;
        result.frame.channels = frame.channels;
        result.frame.sample_format = frame.sample_format;
    }

    result.success = true;
    return result;
}

DecodeResult RealSoftwareDecodeRuntime::flush(const std::string& trace_id) {
    DecodeResult result;
    result.metadata.trace_id = trace_id;

    if (!is_open_ || !codec_context_) {
        result.success = false;
        result.error_message = "not_open: Decoder is not open";
        return result;
    }

    FfmpegCodecHandle codec;
    codec.native = codec_context_;
    FfmpegAdapter::flush_decoder(codec);

    is_flushed_ = true;
    result.success = true;
    result.eof = true;
    return result;
}

void RealSoftwareDecodeRuntime::close() {
    if (codec_context_) {
        FfmpegCodecHandle handle;
        handle.native = codec_context_;
        FfmpegAdapter::free_codec_context(handle);
        codec_context_ = nullptr;
    }
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
