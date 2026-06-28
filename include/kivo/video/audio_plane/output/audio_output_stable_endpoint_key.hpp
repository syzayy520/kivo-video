// audio_output_stable_endpoint_key.hpp — P6A §35: stable endpoint key
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
struct AudioOutputStableEndpointKeyContract {
    ContractMetadata metadata;
    uint64_t salted_endpoint_hash{0};
    uint32_t collision_fallback_index{0};
    uint32_t privacy_rotation_counter{0};
    bool rotation_may_invalidate_persistent_diagnostics{true};
    bool rotation_must_not_break_active_session{true};
};
}  // namespace kivo::video::audio_plane::output
