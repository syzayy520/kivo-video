// ffmpeg_decode_lifecycle.cpp — P6C backend: FFmpegDecodeLifecycleController implementation
// State transition validation using P6A AudioDecodeBackendState (§11 adjacency map).
#include "video/audio_plane/decode/ffmpeg/ffmpeg_decode_lifecycle.hpp"

namespace kivo::video::audio_plane::backend::decode::ffmpeg {

using kivo::video::audio_plane::decode::AudioDecodeBackendState;

// Adjacency map (§11). Same as P6B FakeDecodeBackend.
// Any → Superseded, Any → Closed (universal).
// Other transitions are state-specific.
bool FFmpegDecodeLifecycleController::is_adjacent(
    AudioDecodeBackendState from, AudioDecodeBackendState to) noexcept {
    // Universal transitions
    if (to == AudioDecodeBackendState::Superseded ||
        to == AudioDecodeBackendState::Closed) {
        return true;
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

kivo::Expected<std::monostate, LifecycleTransitionError>
FFmpegDecodeLifecycleController::transition(
    AudioDecodeBackendState from, AudioDecodeBackendState to) {
    // Check if source is terminal
    if (kivo::video::audio_plane::decode::AudioDecodeBackendLifecycle{ {}, from }.is_terminal()) {
        return LifecycleTransitionError::SourceIsTerminal;
    }
    // Check if target is terminal (only Closed/FailedFatal/Superseded are terminal;
    // but they ARE valid targets via universal transitions, so this is informational).
    if (!is_adjacent(from, to)) {
        return LifecycleTransitionError::InvalidAdjacency;
    }
    if (cancelled_) {
        return LifecycleTransitionError::Cancelled;
    }
    return std::monostate{};
}

bool FFmpegDecodeLifecycleController::check_timeout(uint64_t current_ms) const noexcept {
    if (!deadline_set_) return false;
    return current_ms >= deadline_ms_;
}

void FFmpegDecodeLifecycleController::set_deadline_ms(uint32_t ms) noexcept {
    deadline_ms_ = ms;
    deadline_set_ = true;
}

void FFmpegDecodeLifecycleController::cancel(CancelReason reason) noexcept {
    cancelled_ = true;
    last_cancel_reason_ = reason;
}

void FFmpegDecodeLifecycleController::supersede(uint64_t new_generation) noexcept {
    generation_ = new_generation;
    cancelled_ = true;
    last_cancel_reason_ = CancelReason::Superseded;
}

bool FFmpegDecodeLifecycleController::is_cancellable(
    AudioDecodeBackendState state) const noexcept {
    return !kivo::video::audio_plane::decode::AudioDecodeBackendLifecycle{ {}, state }.is_terminal();
}

}  // namespace kivo::video::audio_plane::backend::decode::ffmpeg
