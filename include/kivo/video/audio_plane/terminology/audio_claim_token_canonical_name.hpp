// audio_claim_token_canonical_name.hpp — P6A terminology: claim token name (§terminology)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::terminology {

// AudioClaimTokenCanonicalName
// Canonical names of audio claim tokens. Compile-only skeleton.
struct AudioClaimTokenCanonicalName {
    kivo::video::audio_plane::ContractMetadata metadata{};
    const char* bit_perfect_pcm{"bit_perfect_pcm"};
    const char* passthrough_dolby_digital{"passthrough_dolby_digital"};
    const char* passthrough_dts{"passthrough_dts"};
    const char* passthrough_atmos{"passthrough_atmos"};

    bool operator==(const AudioClaimTokenCanonicalName&) const = default;
};

}  // namespace kivo::video::audio_plane::terminology
