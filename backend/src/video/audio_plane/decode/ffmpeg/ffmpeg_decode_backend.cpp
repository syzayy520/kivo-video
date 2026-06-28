// ffmpeg_decode_backend.cpp — P6C backend: FFmpegDecodeBackend implementation
// Real audio decode backend using libavcodec (avcodec layer only — NO avformat).
#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_backend.hpp"

#include <new>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_lifecycle.hpp"
#include "video/audio_plane/decode/ffmpeg/ffmpeg_frame_converter.hpp"
#include "video/audio_plane/decode/ffmpeg/ffmpeg_codec_private_data.hpp"
#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_evidence_emitter.hpp"
#include "ffmpeg_codec_context_guard.hpp"
#include "ffmpeg_frame_guard.hpp"
#include "ffmpeg_packet_guard.hpp"
#include "ffmpeg_codec_registry.hpp"
#include "ffmpeg_decode_config.hpp"

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::AudioDecodeBackendState;
using kivo::video::audio_plane::decode::DecodedAudioFrame;
using internal::AVCodecContextGuard;
using internal::AVFrameGuard;
using internal::AVPacketGuard;
using internal::FFmpegDecodeConfig;

struct FFmpegDecodeBackend::Impl {
    AVCodecContext* codec_ctx{nullptr};
    AVCodecContextGuard ctx_guard;
    FFmpegDecodeLifecycleController lifecycle;
    FFmpegFrameConverter frame_converter;
    FFmpegCodecPrivateDataAdapter cpd_adapter;
    FFmpegDecodeEvidenceEmitter evidence_emitter;
    FFmpegDecodeConfig current_config;
    AudioDecodeBackendState state{AudioDecodeBackendState::NotCreated};
    uint64_t deadline_ms{0};
    bool deadline_set{false};
    bool deadline_exceeded{false};
    int32_t codec_id{0};
    uint64_t frame_counter{0};
};

FFmpegDecodeBackend::FFmpegDecodeBackend()
    : impl_(std::make_unique<Impl>()) {}

FFmpegDecodeBackend::~FFmpegDecodeBackend() { close(); }

FFmpegDecodeBackend::FFmpegDecodeBackend(FFmpegDecodeBackend&&) noexcept = default;
FFmpegDecodeBackend& FFmpegDecodeBackend::operator=(FFmpegDecodeBackend&&) noexcept = default;

kivo::Expected<std::monostate, InitError> FFmpegDecodeBackend::init(
    int32_t codec_id, uint64_t codec_private_data_ref) {
    if (impl_->state != AudioDecodeBackendState::NotCreated) {
        return InitError::InvalidState;
    }

    // Check codec support BEFORE state transitions (fail fast on bad codec_id).
    if (!internal::is_supported(codec_id)) return InitError::UnsupportedCodec;
    AVCodecID avcodec_id = internal::to_avcodec_id(codec_id);
    const AVCodec* codec = avcodec_find_decoder(avcodec_id);
    if (!codec) return InitError::NoCodecFound;

    // NotCreated → Created
    auto t1 = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Created);
    if (t1.is_error()) return InitError::InvalidState;
    impl_->state = AudioDecodeBackendState::Created;

    // Created → WaitingCodecPrivateData
    auto t2 = impl_->lifecycle.transition(impl_->state,
        AudioDecodeBackendState::WaitingCodecPrivateData);
    if (t2.is_error()) return InitError::InvalidState;
    impl_->state = AudioDecodeBackendState::WaitingCodecPrivateData;

    // Resolve codec private data (ref=0 returns empty vector = no extradata)
    auto cpd = impl_->cpd_adapter.load(codec_private_data_ref);
    if (cpd.is_error()) return InitError::CodecPrivateDataLoadFailed;

    // WaitingCodecPrivateData → Opening
    auto t3 = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Opening);
    if (t3.is_error()) return InitError::InvalidState;
    impl_->state = AudioDecodeBackendState::Opening;

    // Allocate context
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (!ctx) return InitError::AllocContextFailed;
    impl_->codec_ctx = ctx;
    impl_->ctx_guard.take(ctx);

    // Apply codec private data (extradata)
    impl_->cpd_adapter.apply_to_context(ctx, cpd.value());

    // Open codec
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        return InitError::OpenFailed;
    }

    // Extract config from opened context
    impl_->current_config = internal::extract_from_context(ctx);
    impl_->codec_id = codec_id;

    // Opening → Ready
    auto t4 = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Ready);
    if (t4.is_error()) return InitError::InvalidState;
    impl_->state = AudioDecodeBackendState::Ready;

    return std::monostate{};
}

kivo::Expected<std::monostate, DecodeReject> FFmpegDecodeBackend::submit_packet(
    uint64_t packet_handle_id, const uint8_t* data, int size, int64_t pts) {
    if (impl_->state == AudioDecodeBackendState::Closed) return DecodeReject::Closed;
    if (impl_->state == AudioDecodeBackendState::Superseded) return DecodeReject::Superseded;
    if (impl_->state == AudioDecodeBackendState::Draining) return DecodeReject::Draining;
    if (impl_->state == AudioDecodeBackendState::Flushing) return DecodeReject::Flushing;
    if (!data || size <= 0) return DecodeReject::InvalidPacket;
    if (impl_->state != AudioDecodeBackendState::Ready &&
        impl_->state != AudioDecodeBackendState::Decoding) {
        return DecodeReject::NotReady;
    }

    AVPacketGuard pkt_guard;
    pkt_guard.set_data(data, size);
    pkt_guard.get()->pts = pts;

    int ret = avcodec_send_packet(impl_->codec_ctx, pkt_guard.get());
    if (ret < 0) return DecodeReject::SendFailed;

    // Ready/Decoding → Decoding
    if (impl_->state == AudioDecodeBackendState::Ready) {
        auto t = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Decoding);
        if (!t.is_error()) impl_->state = AudioDecodeBackendState::Decoding;
    }
    return std::monostate{};
}

kivo::Expected<DecodedAudioFrame, DecodeStatus> FFmpegDecodeBackend::receive_frame(
    uint64_t pool_slot_id) {
    if (impl_->state == AudioDecodeBackendState::Closed) return DecodeStatus::Closed;
    if (impl_->state == AudioDecodeBackendState::Superseded) return DecodeStatus::Superseded;
    if (impl_->deadline_exceeded) return DecodeStatus::TimedOut;
    if (impl_->state == AudioDecodeBackendState::Ready) return DecodeStatus::NotReady;

    AVFrameGuard frame_guard;
    int ret = avcodec_receive_frame(impl_->codec_ctx, frame_guard.get());
    if (ret == AVERROR(EAGAIN)) return DecodeStatus::WouldBlock;
    if (ret == AVERROR_EOF) return DecodeStatus::Eos;
    if (ret < 0) return DecodeStatus::ReceiveFailed;

    // Detect format change
    if (impl_->frame_converter.detect_format_changed(frame_guard.get(), impl_->current_config)) {
        impl_->current_config = internal::extract_from_context(impl_->codec_ctx);
    }

    DecodedAudioFrame decoded = impl_->frame_converter.convert(frame_guard.get(), pool_slot_id);
    decoded.frame_id = ++impl_->frame_counter;
    return decoded;
}

kivo::Expected<std::monostate, FlushError> FFmpegDecodeBackend::flush() {
    if (impl_->state == AudioDecodeBackendState::Closed) return FlushError::Closed;
    if (impl_->state == AudioDecodeBackendState::Superseded) return FlushError::Superseded;
    if (impl_->state != AudioDecodeBackendState::Decoding &&
        impl_->state != AudioDecodeBackendState::Flushing &&
        impl_->state != AudioDecodeBackendState::Ready) {
        return FlushError::InvalidState;
    }
    // From Ready: flush is a no-op (no decode in progress), just call avcodec_flush_buffers.
    // From Decoding: transition Decoding → Flushing → Ready.
    if (impl_->state == AudioDecodeBackendState::Decoding) {
        auto t = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Flushing);
        if (t.is_error()) return FlushError::InvalidState;
        impl_->state = AudioDecodeBackendState::Flushing;
    }
    avcodec_flush_buffers(impl_->codec_ctx);
    // Flushing → Ready (or stay Ready if already Ready)
    if (impl_->state == AudioDecodeBackendState::Flushing) {
        auto t2 = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Ready);
        if (t2.is_error()) return FlushError::InvalidState;
        impl_->state = AudioDecodeBackendState::Ready;
    }
    return std::monostate{};
}

kivo::Expected<std::monostate, DrainError> FFmpegDecodeBackend::drain() {
    if (impl_->state == AudioDecodeBackendState::Closed) return DrainError::Closed;
    if (impl_->state == AudioDecodeBackendState::Superseded) return DrainError::Superseded;
    if (impl_->state != AudioDecodeBackendState::Decoding &&
        impl_->state != AudioDecodeBackendState::Draining) {
        return DrainError::InvalidState;
    }
    if (impl_->state != AudioDecodeBackendState::Draining) {
        auto t = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Draining);
        if (t.is_error()) return DrainError::InvalidState;
        impl_->state = AudioDecodeBackendState::Draining;
    }
    // Send NULL packet to begin drain
    int ret = avcodec_send_packet(impl_->codec_ctx, nullptr);
    if (ret < 0) return DrainError::SendEosFailed;
    // Loop receive until EOF
    AVFrameGuard frame_guard;
    while (true) {
        ret = avcodec_receive_frame(impl_->codec_ctx, frame_guard.get());
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) return DrainError::SendEosFailed;
        ++impl_->frame_counter;
    }
    // Draining → Ready
    auto t2 = impl_->lifecycle.transition(impl_->state, AudioDecodeBackendState::Ready);
    if (t2.is_error()) return DrainError::InvalidState;
    impl_->state = AudioDecodeBackendState::Ready;
    return std::monostate{};
}

void FFmpegDecodeBackend::close() {
    if (impl_->state == AudioDecodeBackendState::Closed) return;
    impl_->ctx_guard.reset();
    impl_->codec_ctx = nullptr;
    impl_->state = AudioDecodeBackendState::Closed;
}

kivo::Expected<std::monostate, TransitionError> FFmpegDecodeBackend::transition_to(
    AudioDecodeBackendState target) {
    if (kivo::video::audio_plane::decode::AudioDecodeBackendLifecycle{ {}, impl_->state }.is_terminal()) {
        return TransitionError::SourceIsTerminal;
    }
    auto t = impl_->lifecycle.transition(impl_->state, target);
    if (t.is_error()) {
        return TransitionError::InvalidAdjacency;
    }
    impl_->state = target;
    return std::monostate{};
}

AudioDecodeBackendState FFmpegDecodeBackend::state() const noexcept {
    return impl_->state;
}

void FFmpegDecodeBackend::set_deadline_ms(uint32_t ms) noexcept {
    impl_->lifecycle.set_deadline_ms(ms);
    impl_->deadline_set = true;
}

void FFmpegDecodeBackend::exceed_deadline() noexcept {
    impl_->deadline_exceeded = true;
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
