// audio_output_device_id.hpp — P6A §35: internal device identity
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
struct AudioOutputDeviceId { uint64_t value{0}; bool operator==(const AudioOutputDeviceId&) const = default; bool is_valid() const { return value != 0; } };
struct AudioOutputStableEndpointKey {
    ContractMetadata metadata;
    AudioOutputDeviceId device_id;
    uint64_t salted_endpoint_hash{0};
    uint32_t collision_fallback_index{0};
    uint32_t privacy_rotation_counter{0};
};
}  // namespace kivo::video::audio_plane::output
