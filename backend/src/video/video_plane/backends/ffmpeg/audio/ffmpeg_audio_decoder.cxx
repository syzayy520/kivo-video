#include "video/video_plane/backends/ffmpeg/audio/ffmpeg_audio_decoder.hpp"

#include <cerrno>
#include <cstring>
#include <limits>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
}

namespace kivo::video::ffmpeg {
using core::decode::DecodeOutcome;
namespace {
DecodeOutcome failed(const std::string& reason) {
    return {DecodeOutcome::Code::kFailed, reason};
}
DecodeOutcome result(int rc, const char* operation) {
    if (rc >= 0) return {};
    if (rc == AVERROR(EAGAIN)) return {DecodeOutcome::Code::kTryAgain, {}};
    if (rc == AVERROR_EOF) return {DecodeOutcome::Code::kEndOfStream, {}};
    char detail[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(rc, detail, sizeof(detail));
    return failed(std::string("ffmpeg audio ") + operation + ": " + detail);
}
}

struct FfmpegAudioDecoder::Impl {
    AVCodecContext* context = nullptr;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    core::audio::PcmFormat declared{}, output{};
    std::vector<std::uint8_t> pcm;
    bool started = false;
    bool diverged = false;
    DecodeOutcome open_context(const AVCodec* codec) {
        auto* next = avcodec_alloc_context3(codec);
        if (!next) return failed("ffmpeg audio context allocation failed");
        next->sample_rate = declared.sample_rate;
        next->pkt_timebase = AVRational{1, 1'000'000};
        av_channel_layout_default(&next->ch_layout, declared.channel_count);
        const auto opened = result(avcodec_open2(next, codec, nullptr), "open");
        if (!opened.ok()) { avcodec_free_context(&next); return opened; }
        avcodec_free_context(&context);
        context = next;
        return {};
    }
    ~Impl() {
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&context);
    }
};

FfmpegAudioDecoder::FfmpegAudioDecoder() : impl_(std::make_unique<Impl>()) {}
FfmpegAudioDecoder::~FfmpegAudioDecoder() = default;

DecodeOutcome FfmpegAudioDecoder::configure(const std::string& mime,
                                           const core::audio::PcmFormat& declared) {
    auto& s = *impl_;
    if (s.context) return failed("ffmpeg audio already configured");
    if (!declared.is_valid() || declared.channel_count > 8)
        return failed("ffmpeg audio invalid declared PCM format");
    const auto id = mime == "audio/eac3" ? AV_CODEC_ID_EAC3 :
                    mime == "audio/ac3" ? AV_CODEC_ID_AC3 :
                    mime == "audio/true-hd" ? AV_CODEC_ID_TRUEHD :
                    mime == "audio/vnd.dts" ? AV_CODEC_ID_DTS : AV_CODEC_ID_NONE;
    const auto* codec = avcodec_find_decoder(id);
    if (!codec) return failed("ffmpeg audio decoder unavailable for " + mime);
    if (!s.packet || !s.frame) return failed("ffmpeg audio allocation failed");
    s.declared = declared;
    return s.open_context(codec);
}

DecodeOutcome FfmpegAudioDecoder::start() {
    if (!impl_->context || !avcodec_is_open(impl_->context))
        return failed("ffmpeg audio start before configure");
    impl_->started = true;
    return {};
}

DecodeOutcome FfmpegAudioDecoder::submit(const core::decode::SampleView& sample, std::int64_t) {
    auto& s = *impl_;
    if (!s.started) return failed("ffmpeg audio submit before start");
    if (sample.is_encrypted) return failed("encrypted audio sample is not supported");
    if (!sample.data || sample.size == 0 || sample.size > std::numeric_limits<int>::max())
        return failed("ffmpeg audio invalid compressed packet size");
    av_packet_unref(s.packet);
    // av_new_packet adds and zeroes AV_INPUT_BUFFER_PADDING_SIZE; source has no such guarantee.
    auto allocated = result(av_new_packet(s.packet, static_cast<int>(sample.size)), "packet allocation");
    if (!allocated.ok()) return allocated;
    std::memcpy(s.packet->data, sample.data, sample.size);
    s.packet->pts = s.packet->dts = sample.presentation_time_us;
    auto sent = result(avcodec_send_packet(s.context, s.packet), "send packet");
    av_packet_unref(s.packet); // send owns its reference; feeder retains source packet on EAGAIN.
    return sent;
}

DecodeOutcome FfmpegAudioDecoder::signal_end_of_stream(std::int64_t) {
    if (!impl_->started) return failed("ffmpeg audio EOS before start");
    return result(avcodec_send_packet(impl_->context, nullptr), "send EOS");
}

DecodeOutcome FfmpegAudioDecoder::receive(core::audio::PcmChunk& out, std::int64_t) {
    out = {};
    auto& s = *impl_;
    if (!s.started) return failed("ffmpeg audio receive before start");
    av_frame_unref(s.frame);
    auto received = result(avcodec_receive_frame(s.context, s.frame), "receive");
    if (!received.ok()) return received;
    const auto& f = *s.frame;
    const int channels = f.ch_layout.nb_channels;
    // Native order is ascending speaker bits (FL/FR/FC/LFE/...); never accept custom ordering.
    if (f.ch_layout.order != AV_CHANNEL_ORDER_NATIVE || channels < 1 || channels > 8 ||
        !av_channel_layout_check(&f.ch_layout))
        return failed("ffmpeg audio unsupported channel layout (no implicit remix)");
    const auto mask = f.ch_layout.u.mask;
    if (mask != AV_CH_LAYOUT_MONO && mask != AV_CH_LAYOUT_STEREO && mask != AV_CH_LAYOUT_5POINT1)
        return failed("ffmpeg audio supports mono, stereo and 5.1(side) only; channel mapping unavailable");
    const auto format = static_cast<AVSampleFormat>(f.format);
    const auto packed = av_get_packed_sample_fmt(format);
    const auto encoding = packed == AV_SAMPLE_FMT_FLT ? core::audio::PcmEncoding::kFloat32 :
                          packed == AV_SAMPLE_FMT_S16 ? core::audio::PcmEncoding::kInt16 :
                          packed == AV_SAMPLE_FMT_S32 ? core::audio::PcmEncoding::kInt32 :
                          core::audio::PcmEncoding::kUnknown;
    s.output = {f.sample_rate, channels, encoding};
    if (!s.output.is_valid() || f.nb_samples <= 0 || f.nb_samples > 1'048'576)
        return failed("ffmpeg audio unsupported PCM sample format or size");
    s.diverged = s.output.sample_rate != s.declared.sample_rate || channels != s.declared.channel_count;
    if (s.diverged) return failed("ffmpeg audio output rate/channels differ from container; no implicit remix");
    const auto bytes = static_cast<std::size_t>(s.output.bytes_per_sample());
    s.pcm.resize(static_cast<std::size_t>(f.nb_samples) * channels * bytes);
    if (!f.extended_data || !f.extended_data[0]) return failed("ffmpeg audio PCM data missing");
    if (av_sample_fmt_is_planar(format)) {
        for (int c = 0; c < channels; ++c) {
            if (!f.extended_data[c]) return failed("ffmpeg audio PCM plane missing");
            for (int n = 0; n < f.nb_samples; ++n)
                std::memcpy(s.pcm.data() + (static_cast<std::size_t>(n) * channels + c) * bytes,
                            f.extended_data[c] + static_cast<std::size_t>(n) * bytes, bytes);
        }
    } else {
        std::memcpy(s.pcm.data(), f.extended_data[0], s.pcm.size());
    }
    out = {s.pcm.data(), s.pcm.size(), f.pts == AV_NOPTS_VALUE ? -1 : f.pts, f.nb_samples};
    return {};
}

DecodeOutcome FfmpegAudioDecoder::flush() {
    if (!impl_->started) return failed("ffmpeg audio flush before start");
    // This bundled AC3/EAC3 implementation has no codec flush callback. Generic
    // avcodec_flush_buffers leaves its IMDCT overlap and dither history alive.
    // Reopen only the decoder, keeping the output device/clock stable across seek.
    const auto reopened = impl_->open_context(impl_->context->codec);
    if (!reopened.ok()) return reopened;
    av_packet_unref(impl_->packet);
    av_frame_unref(impl_->frame);
    impl_->output = {};
    impl_->diverged = false;
    return {};
}
void FfmpegAudioDecoder::stop() { impl_->started = false; }
std::string FfmpegAudioDecoder::codec_name() const {
    return impl_->context ? std::string("ffmpeg:") + impl_->context->codec->name : std::string{};
}
const core::audio::PcmFormat& FfmpegAudioDecoder::output_format() const { return impl_->output; }
bool FfmpegAudioDecoder::output_format_diverged() const { return impl_->diverged; }

}  // namespace kivo::video::ffmpeg
