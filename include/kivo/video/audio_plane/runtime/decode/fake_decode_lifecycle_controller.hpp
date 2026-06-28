// fake_decode_lifecycle_controller.hpp — P6B runtime: fake decode lifecycle controller
// HEADER-ONLY fake implementation. Platform-neutral: no WASAPI/FFmpeg/Qt/Win32/COM.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"

namespace kivo::video::audio_plane::runtime::decode {

using kivo::video::audio_plane::decode::AudioDecodeBackendState;

// LifecycleTransitionError: why a lifecycle transition was rejected.
enum class LifecycleTransitionError : int32_t {
    InvalidAdjacency = 0,
    SourceIsTerminal = 1,
    Cancelled = 2,
    Superseded = 3,
};

// CancelReason: why the controller was cancelled.
enum class CancelReason : int32_t {
    UserRequested = 0,
    GenerationSuperseded = 1,
    FatalError = 2,
};

// FakeDecodeLifecycleController
// Validates allowed transitions per §11 adjacency map and supports
// cancel/supersede. Timeout thresholds are configurable.
// This is a FAKE ADAPTER: no real codec lifecycle, no real init.
class FakeDecodeLifecycleController {
public:
    FakeDecodeLifecycleController() = default;

    // Transition from_state → to_state. Validates adjacency.
    kivo::Expected<std::monostate, LifecycleTransitionError> transition(
        AudioDecodeBackendState from_state,
        AudioDecodeBackendState to_state) {
        if (cancelled_) return LifecycleTransitionError::Cancelled;
        if (superseded_) return LifecycleTransitionError::Superseded;
        decode::AudioDecodeBackendLifecycle lifecycle{};
        lifecycle.state = from_state;
        if (lifecycle.is_terminal()) {
            return LifecycleTransitionError::SourceIsTerminal;
        }
        if (!is_adjacent(from_state, to_state)) {
            return LifecycleTransitionError::InvalidAdjacency;
        }
        current_state_ = to_state;
        return std::monostate{};
    }

    // Cancel init/decode and release handles. After cancel, all transitions fail.
    void cancel(CancelReason /*reason*/) noexcept {
        cancelled_ = true;
        current_state_ = AudioDecodeBackendState::Closed;
    }

    // Supersede by a new generation. Must not publish frames after.
    void supersede(uint64_t /*new_generation*/) noexcept {
        superseded_ = true;
        current_state_ = AudioDecodeBackendState::Superseded;
    }

    // Is the given state cancellable (non-terminal)?
    bool is_cancellable(AudioDecodeBackendState state) const noexcept {
        decode::AudioDecodeBackendLifecycle lifecycle{};
        lifecycle.state = state;
        return !lifecycle.is_terminal();
    }

    AudioDecodeBackendState current_state() const noexcept { return current_state_; }
    bool is_cancelled() const noexcept { return cancelled_; }
    bool is_superseded() const noexcept { return superseded_; }

    // Configurable timeout thresholds (ms).
    uint32_t normal_timeout_ms() const noexcept { return normal_timeout_ms_; }
    uint32_t live_timeout_ms() const noexcept { return live_timeout_ms_; }
    void set_normal_timeout_ms(uint32_t ms) noexcept { normal_timeout_ms_ = ms; }
    void set_live_timeout_ms(uint32_t ms) noexcept { live_timeout_ms_ = ms; }

private:
    // Adjacency map per §11 (mirrors FakeDecodeBackend's map).
    static bool is_adjacent(
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

    AudioDecodeBackendState current_state_{AudioDecodeBackendState::NotCreated};
    uint32_t normal_timeout_ms_{3000};
    uint32_t live_timeout_ms_{5000};
    bool cancelled_{false};
    bool superseded_{false};
};

}  // namespace kivo::video::audio_plane::runtime::decode
