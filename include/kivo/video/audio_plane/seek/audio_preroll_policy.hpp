// audio_preroll_policy.hpp — P6A seek: preroll policy contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::seek {
struct AudioPrerollPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    int32_t max_preroll_ms{500};
    bool keyframe_required{true};
};
}  // namespace kivo::video::audio_plane::seek
