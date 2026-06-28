// audio_output_state.hpp — P6A §5: output state
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/output/audio_output_mode.hpp"
#include "kivo/video/audio_plane/output/audio_output_backend_lifecycle.hpp"
namespace kivo::video::audio_plane::output {
struct AudioOutputState {
    ContractMetadata metadata;
    AudioOutputMode mode{AudioOutputMode::Unknown};
    AudioOutputBackendState backend_state{AudioOutputBackendState::NotCreated};
    bool is_audible{false};
    bool is_underfill{false};
    int64_t last_position_ms{0};
};
}  // namespace kivo::video::audio_plane::output
