// audio_output_state_snapshot.hpp — P6A §5: immutable state snapshot
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/output/audio_output_state.hpp"
#include "kivo/video/audio_plane/generation/audio_device_generation.hpp"
namespace kivo::video::audio_plane::output {
struct AudioOutputStateSnapshot {
    ContractMetadata metadata;
    AudioOutputState state;
    generation::AudioDeviceGeneration device_generation;
    int64_t qpc_timestamp{0};
    bool is_fresh{false};
};
}  // namespace kivo::video::audio_plane::output
