// dolby_dts_atmos_claim_state.hpp — P6A security: Dolby/DTS/Atmos claim (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/security/audio_codec_claim_policy.hpp"
namespace kivo::video::audio_plane::security {

// DolbyDtsAtmosClaimState
// Aggregated claim state for Dolby, DTS and Atmos. Compile-only skeleton.
struct DolbyDtsAtmosClaimState {
    kivo::video::audio_plane::ContractMetadata metadata{};
    CodecClaimState dolby{CodecClaimState::DetectedOnly};
    CodecClaimState dts{CodecClaimState::DetectedOnly};
    CodecClaimState atmos{CodecClaimState::DetectedOnly};

    bool operator==(const DolbyDtsAtmosClaimState&) const = default;
};

}  // namespace kivo::video::audio_plane::security
