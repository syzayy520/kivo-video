// ffmpeg_decode_backend.hpp — P6C backend: FFmpeg audio decode backend (private interface)
// Private interface: FFmpeg types ARE allowed here (AVCodecContext* etc.) but NOT in public.
// avformat/demux FORBIDDEN — avcodec layer only.
#pragma once

#include <cstdint>
#include <memory>

#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"
#include "kivo/video/audio_plane/decode/decoded_audio_frame.hpp"

// Forward declarations of FFmpeg types (avcodec layer only — NO avformat).
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::AudioDecodeBackendState;
using kivo::video::audio_plane::decode::DecodedAudioFrame;

// InitError: why init() failed.
enum class InitError : int32_t {
    UnsupportedCodec = 0,
    NoCodecFound = 1,
    AllocContextFailed = 2,
    OpenFailed = 3,
    InvalidState = 4,
    CodecPrivateDataLoadFailed = 5,
};

// DecodeReject: why a submitted packet was rejected.
enum class DecodeReject : int32_t {
    NotReady = 0,
    Draining = 1,
    Flushing = 2,
    Closed = 3,
    Superseded = 4,
    InvalidPacket = 5,
    SendFailed = 6,
};

// DecodeStatus: status returned by receive_frame when no frame is produced.
enum class DecodeStatus : int32_t {
    WouldBlock = 0,
    TimedOut = 1,
    Eos = 2,
    NotReady = 3,
    Closed = 4,
    Superseded = 5,
    ReceiveFailed = 6,
};

// FlushError: why flush() failed.
enum class FlushError : int32_t {
    NotReady = 0,
    Closed = 1,
    Superseded = 2,
    InvalidState = 3,
};

// DrainError: why drain() failed.
enum class DrainError : int32_t {
    NotReady = 0,
    Closed = 1,
    Superseded = 2,
    InvalidState = 3,
    SendEosFailed = 4,
};

// TransitionError: why a state transition was rejected.
enum class TransitionError : int32_t {
    InvalidAdjacency = 0,
    SourceIsTerminal = 1,
    TargetIsTerminal = 2,
    Cancelled = 3,
    Superseded = 4,
};

// FFmpegDecodeBackend
// Real audio decode backend using libavcodec (avcodec layer only — NO avformat).
// State machine uses P6A AudioDecodeBackendState (16 states, §7.2).
// All state transitions validated via FFmpegDecodeLifecycleController.
class FFmpegDecodeBackend {
public:
    FFmpegDecodeBackend();
    ~FFmpegDecodeBackend();

    FFmpegDecodeBackend(const FFmpegDecodeBackend&) = delete;
    FFmpegDecodeBackend& operator=(const FFmpegDecodeBackend&) = delete;
    FFmpegDecodeBackend(FFmpegDecodeBackend&&) noexcept;
    FFmpegDecodeBackend& operator=(FFmpegDecodeBackend&&) noexcept;

    // Initialize: NotCreated → Created → WaitingCodecPrivateData → Opening → Ready.
    // codec_id: Kivo codec id (mapped to AVCodecID via internal registry).
    // codec_private_data_ref: P6 data handle ref (resolved via FFmpegCodecPrivateDataAdapter).
    kivo::Expected<std::monostate, InitError> init(
        int32_t codec_id, uint64_t codec_private_data_ref);

    // Submit a packet for decoding. Drives Ready/Decoding → Decoding.
    kivo::Expected<std::monostate, DecodeReject> submit_packet(
        uint64_t packet_handle_id, const uint8_t* data, int size, int64_t pts);

    // Receive the next decoded frame. Returns DecodedAudioFrame on success.
    kivo::Expected<DecodedAudioFrame, DecodeStatus> receive_frame(uint64_t pool_slot_id);

    // Flush: avcodec_flush_buffers. Decoding/Flushing → Flushing → Ready.
    kivo::Expected<std::monostate, FlushError> flush();

    // Drain: send NULL packet, loop receive until EOF. Decoding/Draining → Draining → Ready.
    kivo::Expected<std::monostate, DrainError> drain();

    // Close: release all resources. Any → Closed.
    void close();

    // Transition to an explicit target state. Validates adjacency.
    kivo::Expected<std::monostate, TransitionError> transition_to(
        AudioDecodeBackendState target);

    AudioDecodeBackendState state() const noexcept;
    void set_deadline_ms(uint32_t ms) noexcept;
    void exceed_deadline() noexcept;

private:
    // Pimpl to hide FFmpeg members from this private interface header
    // (keeps include surface minimal; full FFmpeg usage lives in .cpp).
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
