// audio_recovery_state.hpp — P6A recovery: recovery state (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioRecoveryState
// State of a recovery attempt. Compile-only enum.
enum class AudioRecoveryState {
    NotStarted = 0,
    InProgress = 1,
    Recovered = 2,
    Failed = 3
};

// AudioRecoveryStateSnapshot
// Snapshot of recovery state with attempt count. Compile-only skeleton.
struct AudioRecoveryStateSnapshot {
    kivo::video::audio_plane::ContractMetadata metadata{};
    AudioRecoveryState state{AudioRecoveryState::NotStarted};
    int32_t attempts{0};

    bool operator==(const AudioRecoveryStateSnapshot&) const = default;
};

}  // namespace kivo::video::audio_plane::recovery
