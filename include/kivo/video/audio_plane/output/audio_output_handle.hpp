// audio_output_handle.hpp — P6A §35: opaque public handle (no raw endpoint ID)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/platform/audio_platform_backend_id.hpp"
#include "kivo/video/audio_plane/generation/audio_device_generation.hpp"
#include "kivo/video/audio_plane/generation/audio_stream_generation.hpp"
namespace kivo::video::audio_plane::output {
struct AudioRedactedDeviceHandle { uint64_t salted_hash{0}; uint32_t rotation_counter{0}; bool operator==(const AudioRedactedDeviceHandle&) const = default; };
struct AudioOutputHandle {
    ContractMetadata metadata;
    platform::AudioPlatformBackendId backend_id;
    AudioRedactedDeviceHandle redacted_device_handle;
    generation::AudioDeviceGeneration device_generation;
    generation::AudioStreamGeneration stream_generation;
    bool operator==(const AudioOutputHandle&) const = default;
};
}  // namespace kivo::video::audio_plane::output
