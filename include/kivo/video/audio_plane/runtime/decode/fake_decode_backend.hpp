// fake_decode_backend.hpp — P6B runtime: fake audio decode backend
// HEADER-ONLY fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
// FAKE ADAPTER: NOT P6C runtime. No real codec API calls.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"

namespace kivo::video::audio_plane::runtime::decode {

using kivo::video::audio_plane::decode::AudioDecodeBackendState;
using kivo::video::audio_plane::decode::AudioDecodeBackendLifecycle;

// DecodeReject: why a submitted packet was rejected by the fake backend.
enum class DecodeReject : int32_t {
    NotReady = 0,
    Draining = 1,
    Flushing = 2,
    Closed = 3,
    Superseded = 4,
    InvalidPacketId = 5,
};

// DecodeStatus: status returned by receive_frame (not a frame).
enum class DecodeStatus : int32_t {
    WouldBlock = 0,
    TimedOut = 1,
    Eos = 2,
    NotReady = 3,
    Closed = 4,
    Superseded = 5,
};

// TransitionError: why a state transition was rejected.
enum class TransitionError : int32_t {
    InvalidAdjacency = 0,
    SourceIsTerminal = 1,
    TargetIsTerminal = 2,
    Cancelled = 3,
    Superseded = 4,
};

// DecodedFrame: deterministic frame produced by the fake decoder.
// No real codec types, no codec context, no real decoded audio.
struct DecodedFrame {
    uint64_t frame_id{0};
    int32_t sample_count{0};
    int32_t channel_count{0};
    int64_t pts{0};
};

// FakeDecodeBackend
// Deterministic fake of the audio decode backend.
// - State machine uses AudioDecodeBackendState (16 states from P6A §7.2).
// - init() drives NotCreated → Created → Opening → Ready.
// - submit_packet/receive_frame perform deterministic fake decode (counter-based).
// - TimedOut: set_deadline_ms + receive_frame after deadline → TimedOut.
// - This is a FAKE ADAPTER, NOT P6C: no real codec API.
class FakeDecodeBackend {
public:
    FakeDecodeBackend() = default;

    // Initialize: NotCreated → Created → Opening → Ready.
    // Returns TransitionError if any adjacency step is invalid.
    kivo::Expected<std::monostate, TransitionError> init() {
        if (state_ != AudioDecodeBackendState::NotCreated) {
            return TransitionError::InvalidAdjacency;
        }
        if (!transition_adjacent(AudioDecodeBackendState::NotCreated,
                                  AudioDecodeBackendState::Created)) {
            return TransitionError::InvalidAdjacency;
        }
        state_ = AudioDecodeBackendState::Created;
        if (!transition_adjacent(state_, AudioDecodeBackendState::Opening)) {
            return TransitionError::InvalidAdjacency;
        }
        state_ = AudioDecodeBackendState::Opening;
        if (!transition_adjacent(state_, AudioDecodeBackendState::Ready)) {
            return TransitionError::InvalidAdjacency;
        }
        state_ = AudioDecodeBackendState::Ready;
        return std::monostate{};
    }

    // Submit a packet (by id) for decoding. Deterministic: no real decode.
    kivo::Expected<std::monostate, DecodeReject> submit_packet(uint64_t packet_id) {
        if (state_ == AudioDecodeBackendState::Closed) return DecodeReject::Closed;
        if (state_ == AudioDecodeBackendState::Superseded) return DecodeReject::Superseded;
        if (state_ == AudioDecodeBackendState::Draining) return DecodeReject::Draining;
        if (state_ == AudioDecodeBackendState::Flushing) return DecodeReject::Flushing;
        if (packet_id == 0) return DecodeReject::InvalidPacketId;
        if (state_ != AudioDecodeBackendState::Ready &&
            state_ != AudioDecodeBackendState::Decoding) {
            return DecodeReject::NotReady;
        }
        state_ = AudioDecodeBackendState::Decoding;
        ++pending_packet_count_;
        return std::monostate{};
    }

    // Receive the next decoded frame. Deterministic counter-based generation.
    // Returns WouldBlock if no pending packet, TimedOut if deadline exceeded,
    // Eos when no packets remain and state is Draining.
    kivo::Expected<DecodedFrame, DecodeStatus> receive_frame() {
        if (state_ == AudioDecodeBackendState::Closed) return DecodeStatus::Closed;
        if (state_ == AudioDecodeBackendState::Superseded) return DecodeStatus::Superseded;
        if (is_deadline_exceeded_) return DecodeStatus::TimedOut;
        if (state_ == AudioDecodeBackendState::Ready) return DecodeStatus::NotReady;
        if (pending_packet_count_ == 0) {
            if (state_ == AudioDecodeBackendState::Draining) return DecodeStatus::Eos;
            return DecodeStatus::WouldBlock;
        }
        --pending_packet_count_;
        DecodedFrame frame;
        frame.frame_id = ++frame_counter_;
        frame.sample_count = 1024;
        frame.channel_count = 2;
        frame.pts = static_cast<int64_t>(frame_counter_ * 1024);
        return frame;
    }

    // Transition to an explicit target state. Validates adjacency (see §11 map).
    kivo::Expected<std::monostate, TransitionError> transition_to(
        AudioDecodeBackendState target) {
        if (decode::AudioDecodeBackendLifecycle{ {}, state_ }.is_terminal()) {
            return TransitionError::SourceIsTerminal;
        }
        if (!transition_adjacent(state_, target)) {
            return TransitionError::InvalidAdjacency;
        }
        state_ = target;
        return std::monostate{};
    }

    AudioDecodeBackendState state() const noexcept { return state_; }
    uint64_t frame_counter() const noexcept { return frame_counter_; }
    uint64_t pending_packet_count() const noexcept { return pending_packet_count_; }

    // Deadline control for TimedOut simulation.
    void set_deadline_ms(uint32_t /*ms*/) noexcept { deadline_set_ = true; }
    void exceed_deadline() noexcept { is_deadline_exceeded_ = true; }
    bool is_deadline_exceeded() const noexcept { return is_deadline_exceeded_; }

private:
    // Adjacency map (§11). Returns true if from→to is an allowed transition.
    static bool transition_adjacent(
        AudioDecodeBackendState from, AudioDecodeBackendState to) {
        if (to == AudioDecodeBackendState::Superseded ||
            to == AudioDecodeBackendState::Closed) {
            return true;  // Any → Superseded, Any → Closed
        }
        switch (from) {
            case AudioDecodeBackendState::NotCreated:
                return to == AudioDecodeBackendState::Created;
            case AudioDecodeBackendState::Created:
                return to == AudioDecodeBackendState::WaitingCodecPrivateData ||
                       to == AudioDecodeBackendState::Opening;
            case AudioDecodeBackendState::WaitingCodecPrivateData:
                return to == AudioDecodeBackendState::Opening;
            case AudioDecodeBackendState::Opening:
                return to == AudioDecodeBackendState::Ready ||
                       to == AudioDecodeBackendState::TimedOut ||
                       to == AudioDecodeBackendState::FailedRecoverable ||
                       to == AudioDecodeBackendState::FailedFatal;
            case AudioDecodeBackendState::Ready:
                return to == AudioDecodeBackendState::Decoding;
            case AudioDecodeBackendState::Decoding:
                return to == AudioDecodeBackendState::Draining ||
                       to == AudioDecodeBackendState::Flushing ||
                       to == AudioDecodeBackendState::FormatChanging;
            case AudioDecodeBackendState::TimedOut:
                return to == AudioDecodeBackendState::Resetting ||
                       to == AudioDecodeBackendState::FailedFatal;
            case AudioDecodeBackendState::FailedRecoverable:
                return to == AudioDecodeBackendState::Resetting;
            case AudioDecodeBackendState::Resetting:
                return to == AudioDecodeBackendState::Ready;
            default:
                return false;
        }
    }

    AudioDecodeBackendState state_{AudioDecodeBackendState::NotCreated};
    uint64_t frame_counter_{0};
    uint64_t pending_packet_count_{0};
    bool deadline_set_{false};
    bool is_deadline_exceeded_{false};
};

}  // namespace kivo::video::audio_plane::runtime::decode
