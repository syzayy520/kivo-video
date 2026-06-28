// audio_decode_backend_lifecycle.hpp — P6A decode: AudioDecodeBackendState + AudioDecodeBackendLifecycle (§7.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
enum class AudioDecodeBackendState {
    NotCreated = 0,
    Created = 1,
    WaitingCodecPrivateData = 2,
    Opening = 3,
    Ready = 4,
    Decoding = 5,
    Draining = 6,
    Flushing = 7,
    Resetting = 8,
    FormatChanging = 9,
    TimedOut = 10,
    FailedRecoverable = 11,
    FailedFatal = 12,
    Closed = 13,
    Superseded = 14,
    FormatChanged = 15,
};
struct AudioDecodeBackendLifecycle {
    ContractMetadata metadata;
    AudioDecodeBackendState state{AudioDecodeBackendState::NotCreated};
    bool is_terminal() const {
        return state == AudioDecodeBackendState::Closed ||
               state == AudioDecodeBackendState::FailedFatal ||
               state == AudioDecodeBackendState::Superseded;
    }
    bool is_timed_out() const {
        return state == AudioDecodeBackendState::TimedOut;
    }
    bool operator==(const AudioDecodeBackendLifecycle&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
