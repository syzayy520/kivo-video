// ffmpeg_decode_lifecycle.hpp — P6C backend: FFmpeg decode lifecycle controller (private interface)
// Manages state transitions for FFmpegDecodeBackend using P6A AudioDecodeBackendState.
#pragma once

#include <cstdint>

#include "kivo/video/audio_plane/foundation/kivo_expected.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::AudioDecodeBackendState;

// LifecycleTransitionError: why a lifecycle transition was rejected.
enum class LifecycleTransitionError : int32_t {
    InvalidAdjacency = 0,
    SourceIsTerminal = 1,
    TargetIsTerminal = 2,
    Cancelled = 3,
    Superseded = 4,
};

// CancelReason: why the current operation was cancelled.
enum class CancelReason : int32_t {
    UserRequested = 0,
    Timeout = 1,
    Superseded = 2,
    FatalError = 3,
};

// FFmpegDecodeLifecycleController
// Validates and records state transitions for FFmpegDecodeBackend.
// Uses the same adjacency map as P6B FakeDecodeBackend (§11 transitions).
class FFmpegDecodeLifecycleController {
public:
    FFmpegDecodeLifecycleController() = default;

    // Validate and apply a state transition.
    // Returns LifecycleTransitionError if the transition is not allowed.
    kivo::Expected<std::monostate, LifecycleTransitionError> transition(
        AudioDecodeBackendState from, AudioDecodeBackendState to);

    // Returns true if current_ms exceeds the deadline (if one is set).
    bool check_timeout(uint64_t current_ms) const noexcept;

    // Set the deadline in milliseconds (relative to when set).
    void set_deadline_ms(uint32_t ms) noexcept;

    // Cancel the current operation. Forces transition to a failure/terminal state.
    void cancel(CancelReason reason) noexcept;

    // Supersede the current operation with a new generation.
    void supersede(uint64_t new_generation) noexcept;

    // Returns true if the given state is cancellable (not terminal).
    bool is_cancellable(AudioDecodeBackendState state) const noexcept;

    // Returns true if from→to is an allowed transition (§11 adjacency map).
    static bool is_adjacent(
        AudioDecodeBackendState from, AudioDecodeBackendState to) noexcept;

    uint64_t generation() const noexcept { return generation_; }
    bool has_deadline() const noexcept { return deadline_set_; }
    uint64_t deadline_ms() const noexcept { return deadline_ms_; }
    CancelReason last_cancel_reason() const noexcept { return last_cancel_reason_; }

private:
    uint64_t generation_{0};
    uint64_t deadline_ms_{0};
    bool deadline_set_{false};
    bool cancelled_{false};
    CancelReason last_cancel_reason_{CancelReason::UserRequested};
};

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
