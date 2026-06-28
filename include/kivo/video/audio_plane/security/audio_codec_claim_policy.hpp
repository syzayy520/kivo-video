// audio_codec_claim_policy.hpp — P6A security: codec claim policy (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// CodecClaimState
// Lifecycle state of a codec claim. Compile-only enum.
enum class CodecClaimState {
    DetectedOnly = 0,
    Supported = 1,
    BoundaryOnly = 2,
    LicensedForDistribution = 3,
    CertifiedByVendor = 4
};

// AudioCodecClaimPolicy
// Policy binding a codec claim to its state. Compile-only skeleton.
struct AudioCodecClaimPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    CodecClaimState state{CodecClaimState::DetectedOnly};

    bool operator==(const AudioCodecClaimPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::security
