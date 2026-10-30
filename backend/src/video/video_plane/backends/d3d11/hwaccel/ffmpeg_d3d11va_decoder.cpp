#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_d3d11va_decoder.hpp"

#include <d3d11.h>
extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/pixfmt.h>
}

#include <cstring>
#include <utility>

#include "video/video_plane/backends/d3d11/hwaccel/ffmpeg_get_format_policy.hpp"

namespace kivo::video::video_plane::backends::d3d11::hwaccel {
namespace {

namespace gpu = kivo::cinema_engine::video_plane_gpu;

// Installed into AVCodecContext::get_format. Stateless on purpose: the decision
// depends only on what the decoder offers.
auto format_trampoline(AVCodecContext* /*context*/, const AVPixelFormat* offered) -> AVPixelFormat {
    return decide_get_format(offered).format;
}

[[nodiscard]] auto map_sw_format(const int sw_format) noexcept -> gpu::GpuPixelFormat {
    switch (sw_format) {
        case AV_PIX_FMT_NV12:
            return gpu::GpuPixelFormat::Nv12;
        case AV_PIX_FMT_P010:
            return gpu::GpuPixelFormat::P010;
        default:
            return gpu::GpuPixelFormat::Unknown;
    }
}

[[nodiscard]] auto map_codec_name(const std::string& name) noexcept -> gpu::GpuCodec {
    if (name == "h264") {
        return gpu::GpuCodec::H264;
    }
    if (name == "hevc") {
        return gpu::GpuCodec::Hevc;
    }
    if (name == "av1") {
        return gpu::GpuCodec::Av1;
    }
    if (name == "vp9") {
        return gpu::GpuCodec::Vp9;
    }
    if (name == "vc1") {
        return gpu::GpuCodec::Vc1;
    }
    if (name == "mpeg2video") {
        return gpu::GpuCodec::Mpeg2;
    }
    return gpu::GpuCodec::Unknown;
}

[[nodiscard]] auto rescale_to_us(const std::int64_t value, const AVRational time_base) noexcept
    -> std::int64_t {
    if (value == AV_NOPTS_VALUE || time_base.num <= 0 || time_base.den <= 0) {
        return 0;
    }
    return av_rescale_q(value, time_base, AV_TIME_BASE_Q);
}

[[nodiscard]] auto make_error(gpu::DecodeStatus status, std::string message) -> gpu::DecodeOutcome {
    gpu::DecodeOutcome outcome{};
    outcome.status = status;
    outcome.error = std::move(message);
    return outcome;
}

[[nodiscard]] auto make_submit(gpu::SubmitStatus status, std::string message) -> gpu::SubmitOutcome {
    gpu::SubmitOutcome outcome{};
    outcome.status = status;
    outcome.error = std::move(message);
    return outcome;
}

}  // namespace

FfmpegD3d11vaDecoder::~FfmpegD3d11vaDecoder() {
    release_live_frames();
    release_decoder();
}

auto FfmpegD3d11vaDecoder::configure(const gpu::HardwareDecodeConfig& config,
                                     const gpu::GpuDeviceHandle device_handle) -> bool {
    release_live_frames();
    release_decoder();
    last_error_.clear();
    codec_name_ = config.codec_name;

    const AVCodec* codec = avcodec_find_decoder_by_name(config.codec_name.c_str());
    if (codec == nullptr) {
        last_error_ = "decoder not found: " + config.codec_name;
        return false;
    }

    auto* device = static_cast<ID3D11Device*>(device_handle);
    if (device == nullptr) {
        last_error_ = "null gpu device handle";
        return false;
    }

    if (!binding_.bind(device)) {
        last_error_ = binding_.last_error() != nullptr ? binding_.last_error()
                                                      : "hwdevice bind failed";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (codec_ctx_ == nullptr) {
        last_error_ = "avcodec_alloc_context3 failed";
        release_decoder();
        return false;
    }

    codec_ctx_->hw_device_ctx = av_buffer_ref(binding_.ref());
    if (codec_ctx_->hw_device_ctx == nullptr) {
        last_error_ = "av_buffer_ref on hw device failed";
        release_decoder();
        return false;
    }

    codec_ctx_->get_format = &format_trampoline;
    codec_ctx_->thread_count = config.thread_count;
    if (config.coded_width > 0) {
        codec_ctx_->width = config.coded_width;
    }
    if (config.coded_height > 0) {
        codec_ctx_->height = config.coded_height;
    }

    // Container-supplied parameter sets. Without these an MP4/MOV HEVC stream
    // opens fine and then decodes nothing, which is the worst failure mode:
    // it looks like success.
    if (config.extradata != nullptr && config.extradata_size > 0) {
        constexpr std::size_t kPadding = AV_INPUT_BUFFER_PADDING_SIZE;
        codec_ctx_->extradata = static_cast<std::uint8_t*>(
            av_mallocz(config.extradata_size + kPadding));
        if (codec_ctx_->extradata == nullptr) {
            last_error_ = "extradata allocation failed";
            release_decoder();
            return false;
        }
        std::memcpy(codec_ctx_->extradata, config.extradata, config.extradata_size);
        codec_ctx_->extradata_size = static_cast<int>(config.extradata_size);
    }

    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        last_error_ = "avcodec_open2 failed for " + config.codec_name;
        release_decoder();
        return false;
    }

    return true;
}

auto FfmpegD3d11vaDecoder::capability() const -> gpu::GpuDecodeCapability {
    gpu::GpuDecodeCapability capability{};
    capability.decoder_name = codec_name_;

    if (codec_ctx_ == nullptr) {
        capability.hardware_decode_available = false;
        capability.reason_if_unavailable =
            last_error_.empty() ? "decoder not configured" : last_error_;
        return capability;
    }

    capability.hardware_decode_available = true;

    gpu::GpuCodecSupport support{};
    support.codec = map_codec_name(codec_name_);
    support.eight_bit = true;
    support.ten_bit = saw_ten_bit_;
    capability.codecs.push_back(support);
    return capability;
}

auto FfmpegD3d11vaDecoder::submit_packet(const std::uint8_t* data,
                                         const std::size_t size,
                                         const std::int64_t pts,
                                         const std::int64_t dts,
                                         const std::int64_t time_base_num,
                                         const std::int64_t time_base_den) -> gpu::SubmitOutcome {
    if (codec_ctx_ == nullptr) {
        return make_submit(gpu::SubmitStatus::HardwareUnavailable,
                           last_error_.empty() ? "decoder not configured" : last_error_);
    }
    if (data == nullptr || size == 0) {
        return make_submit(gpu::SubmitStatus::Error, "empty packet");
    }
    if (time_base_num > 0 && time_base_den > 0) {
        time_base_ = AVRational{static_cast<int>(time_base_num), static_cast<int>(time_base_den)};
        if (codec_ctx_->pkt_timebase.num == 0) {
            codec_ctx_->pkt_timebase = time_base_;
        }
    }

    AVBufferRef* staging = acquire_staging(size);
    if (staging == nullptr) {
        // Every slot is still referenced by the decoder. Reporting NeedsDrain
        // makes the caller drain and retry instead of corrupting live packets.
        return make_submit(gpu::SubmitStatus::NeedsDrain, "");
    }

    // Copy into a writable staging slot, then hand the decoder a reference to it.
    // Reference counting is atomic and allocation-free, so the steady-state
    // decode loop performs no heap traffic on the submission path.
    constexpr std::size_t kPadding = AV_INPUT_BUFFER_PADDING_SIZE;
    std::memcpy(staging->data, data, size);
    std::memset(staging->data + size, 0, kPadding);

    av_packet_unref(scratch_packet_);
    scratch_packet_->buf = av_buffer_ref(staging);
    if (scratch_packet_->buf == nullptr) {
        return make_submit(gpu::SubmitStatus::Error, "packet buffer ref failed");
    }
    scratch_packet_->data = scratch_packet_->buf->data;
    scratch_packet_->size = static_cast<int>(size);
    scratch_packet_->pts = pts;
    scratch_packet_->dts = dts;

    const int send_result = avcodec_send_packet(codec_ctx_, scratch_packet_);
    av_packet_unref(scratch_packet_);

    if (send_result == AVERROR(EAGAIN)) {
        // The packet was not consumed. Returning Accepted here would drop it and
        // break the reference chain a few frames later ("could not find ref").
        return make_submit(gpu::SubmitStatus::NeedsDrain, "");
    }
    if (send_result == AVERROR_EOF) {
        return make_submit(gpu::SubmitStatus::EndOfStream, "decoder already drained");
    }
    if (send_result < 0) {
        last_error_ = "avcodec_send_packet failed: " + std::to_string(send_result);
        return make_submit(gpu::SubmitStatus::Error, last_error_);
    }
    return make_submit(gpu::SubmitStatus::Accepted, "");
}

auto FfmpegD3d11vaDecoder::acquire_staging(const std::size_t required) -> AVBufferRef* {
    if (scratch_packet_ == nullptr) {
        scratch_packet_ = av_packet_alloc();
        if (scratch_packet_ == nullptr) {
            return nullptr;
        }
    }

    // Prefer an existing slot that nobody else references.
    for (auto& slot : staging_) {
        if (slot.buffer != nullptr && slot.capacity >= required &&
            av_buffer_is_writable(slot.buffer)) {
            return slot.buffer;
        }
    }
    // Otherwise take a free slot.
    for (auto& slot : staging_) {
        if (slot.buffer == nullptr) {
            // Grow with headroom so a slightly larger keyframe does not reallocate.
            const std::size_t capacity =
                required + required / 2 + 65536 + AV_INPUT_BUFFER_PADDING_SIZE;
            slot.buffer = av_buffer_allocz(capacity);
            slot.capacity = slot.buffer != nullptr ? capacity : 0;
            return slot.buffer;
        }
    }
    return nullptr;
}

auto FfmpegD3d11vaDecoder::receive_frame() -> gpu::DecodeOutcome {
    if (codec_ctx_ == nullptr) {
        return make_error(gpu::DecodeStatus::HardwareUnavailable,
                          last_error_.empty() ? "decoder not configured" : last_error_);
    }
    return receive_into_outcome();
}

auto FfmpegD3d11vaDecoder::submit_end_of_stream() -> gpu::SubmitOutcome {
    if (codec_ctx_ == nullptr) {
        return make_submit(gpu::SubmitStatus::HardwareUnavailable,
                           last_error_.empty() ? "decoder not configured" : last_error_);
    }
    const int send_result = avcodec_send_packet(codec_ctx_, nullptr);
    if (send_result == AVERROR(EAGAIN)) {
        return make_submit(gpu::SubmitStatus::NeedsDrain, "");
    }
    if (send_result == AVERROR_EOF) {
        return make_submit(gpu::SubmitStatus::EndOfStream, "");
    }
    if (send_result < 0) {
        last_error_ = "flush rejected: " + std::to_string(send_result);
        return make_submit(gpu::SubmitStatus::Error, last_error_);
    }
    return make_submit(gpu::SubmitStatus::Accepted, "");
}

auto FfmpegD3d11vaDecoder::acquire_live_slot() noexcept -> std::size_t {
    for (std::size_t index = 0; index < live_frames_.size(); ++index) {
        if (!live_frames_[index].in_use) {
            return index;
        }
    }
    return live_frames_.size();
}

auto FfmpegD3d11vaDecoder::receive_into_outcome() -> gpu::DecodeOutcome {
    const std::size_t slot_index = acquire_live_slot();
    if (slot_index >= live_frames_.size()) {
        return make_error(gpu::DecodeStatus::Error,
                          "live frame table full: consumer is not releasing frames");
    }
    LiveFrameSlot& slot = live_frames_[slot_index];
    if (slot.frame == nullptr) {
        slot.frame = av_frame_alloc();
        if (slot.frame == nullptr) {
            return make_error(gpu::DecodeStatus::Error, "av_frame_alloc failed");
        }
    }
    av_frame_unref(slot.frame);

    const int receive_result = avcodec_receive_frame(codec_ctx_, slot.frame);
    if (receive_result == AVERROR(EAGAIN)) {
        return make_error(gpu::DecodeStatus::NeedsMoreInput, "");
    }
    if (receive_result == AVERROR_EOF) {
        return make_error(gpu::DecodeStatus::EndOfStream, "");
    }
    if (receive_result < 0) {
        last_error_ = "avcodec_receive_frame failed: " + std::to_string(receive_result);
        return make_error(gpu::DecodeStatus::Error, last_error_);
    }

    // Hard gate: a frame that is not a D3D11 texture means the decoder quietly
    // went software. Refuse it rather than pass off a CPU frame as hardware.
    if (slot.frame->format != AV_PIX_FMT_D3D11) {
        av_frame_unref(slot.frame);
        return make_error(gpu::DecodeStatus::HardwareUnavailable,
                          "decoder produced a non-hardware frame");
    }

    slot.in_use = true;
    gpu::DecodeOutcome outcome{};
    outcome.status = gpu::DecodeStatus::Frame;
    outcome.frame = make_frame_contract(slot_index);
    if (!outcome.frame.is_valid()) {
        slot.in_use = false;
        av_frame_unref(slot.frame);
        return make_error(gpu::DecodeStatus::Error, "invalid GPU frame contract");
    }
    return outcome;
}

auto FfmpegD3d11vaDecoder::make_frame_contract(const std::size_t slot_index)
    -> gpu::GpuFrameContract {
    gpu::GpuFrameContract contract{};
    AVFrame* frame = live_frames_[slot_index].frame;

    auto* texture = reinterpret_cast<ID3D11Texture2D*>(frame->data[0]);
    const auto array_slice =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(frame->data[1]));

    gpu::GpuPixelFormat format = gpu::GpuPixelFormat::Unknown;
    if (frame->hw_frames_ctx != nullptr) {
        const auto* frames_ctx =
            reinterpret_cast<const AVHWFramesContext*>(frame->hw_frames_ctx->data);
        format = map_sw_format(static_cast<int>(frames_ctx->sw_format));
    }
    if (format == gpu::GpuPixelFormat::P010) {
        saw_ten_bit_ = true;
    }

    // Token is the slot index plus one, so 0 keeps meaning "no frame".
    const auto token = static_cast<std::uint64_t>(slot_index) + 1ULL;

    contract.surface.texture = texture;
    contract.surface.desc.width = frame->width;
    contract.surface.desc.height = frame->height;
    contract.surface.desc.format = format;
    contract.surface.desc.array_slice = array_slice;
    contract.surface.surface_id = token;

    contract.pts_us = rescale_to_us(frame->pts, time_base_);
    contract.duration_us =
        frame->duration > 0 ? rescale_to_us(frame->duration, time_base_) : 0;
    contract.color.primaries = static_cast<std::int32_t>(frame->color_primaries);
    contract.color.transfer = static_cast<std::int32_t>(frame->color_trc);
    contract.color.matrix = static_cast<std::int32_t>(frame->colorspace);
    contract.color.range = static_cast<std::int32_t>(frame->color_range);
    contract.hardware_decoded = true;
    contract.frame_token = token;

    return contract;
}

void FfmpegD3d11vaDecoder::release_frame(const std::uint64_t frame_token) noexcept {
    if (frame_token == 0 || frame_token > live_frames_.size()) {
        return;
    }
    LiveFrameSlot& slot = live_frames_[static_cast<std::size_t>(frame_token - 1)];
    if (!slot.in_use) {
        return;
    }
    if (slot.frame != nullptr) {
        av_frame_unref(slot.frame);
    }
    slot.in_use = false;
}

void FfmpegD3d11vaDecoder::reset(const std::int64_t seek_pts_us) {
    (void)seek_pts_us;
    if (codec_ctx_ != nullptr) {
        avcodec_flush_buffers(codec_ctx_);
    }
    release_live_frames();
}

void FfmpegD3d11vaDecoder::release_decoder() noexcept {
    if (codec_ctx_ != nullptr) {
        avcodec_free_context(&codec_ctx_);
        codec_ctx_ = nullptr;
    }
    if (scratch_packet_ != nullptr) {
        av_packet_free(&scratch_packet_);
    }
    for (auto& slot : staging_) {
        av_buffer_unref(&slot.buffer);
        slot.capacity = 0;
    }
    binding_.release();
    saw_ten_bit_ = false;
}

void FfmpegD3d11vaDecoder::release_live_frames() noexcept {
    for (auto& slot : live_frames_) {
        if (slot.frame != nullptr) {
            av_frame_free(&slot.frame);
        }
        slot.in_use = false;
    }
}

}  // namespace kivo::video::video_plane::backends::d3d11::hwaccel
