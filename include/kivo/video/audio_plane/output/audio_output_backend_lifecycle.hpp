// audio_output_backend_lifecycle.hpp — P6A §13: output backend state machine
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
enum class AudioOutputBackendState : int32_t {
    NotCreated = 0, Created = 1, Opening = 2, Ready = 3, Running = 4,
    Paused = 5, Draining = 6, Flushing = 7, Recovering = 8,
    DeviceLost = 9, Failed = 10, Closed = 11,
};
struct AudioOutputBackendLifecycle {
    ContractMetadata metadata;
    AudioOutputBackendState state{AudioOutputBackendState::NotCreated};
    bool is_terminal() const { return state == AudioOutputBackendState::Closed; }
    bool is_recoverable() const { return state == AudioOutputBackendState::Recovering || state == AudioOutputBackendState::DeviceLost; }
};
}  // namespace kivo::video::audio_plane::output
