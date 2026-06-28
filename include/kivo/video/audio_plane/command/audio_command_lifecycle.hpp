// audio_command_lifecycle.hpp — P6A command: AudioCommandLifecycle state (§6, §23)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::command {
enum class AudioCommandLifecycleState {
    Received,
    Accepted,
    Deferred,
    Executing,
    Executed,
    Failed,
    Superseded,
    Rejected
};
struct AudioCommandLifecycle {
    ContractMetadata metadata;
    AudioCommandLifecycleState state{AudioCommandLifecycleState::Received};
    bool is_terminal() const {
        return state == AudioCommandLifecycleState::Executed
            || state == AudioCommandLifecycleState::Failed
            || state == AudioCommandLifecycleState::Superseded
            || state == AudioCommandLifecycleState::Rejected;
    }
};
}  // namespace kivo::video::audio_plane::command
