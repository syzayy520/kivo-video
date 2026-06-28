// audio_fast_seek_policy.hpp — P6A seek: fast seek policy contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::seek {
struct AudioFastSeekPolicy {
    kivo::video::audio_plane::ContractMetadata metadata;
    bool starts_near_decoder_safe_boundary{true};
    bool reports_actual_audible_error{true};
};
}  // namespace kivo::video::audio_plane::seek
