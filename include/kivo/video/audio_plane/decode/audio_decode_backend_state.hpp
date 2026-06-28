// audio_decode_backend_state.hpp — P6A decode: AudioDecodeBackendStateSnapshot (§7.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/decode/audio_decode_backend_lifecycle.hpp"
namespace kivo::video::audio_plane::decode {
struct AudioDecodeBackendStateSnapshot {
    ContractMetadata metadata;
    AudioDecodeBackendState current_state{AudioDecodeBackendState::NotCreated};
    AudioDecodeBackendState previous_state{AudioDecodeBackendState::NotCreated};
    int64_t state_timestamp_ms{0};
    uint64_t pipeline_generation{0};
    bool operator==(const AudioDecodeBackendStateSnapshot&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
