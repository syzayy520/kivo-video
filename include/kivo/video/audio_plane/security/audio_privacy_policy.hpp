// audio_privacy_policy.hpp — P6A security: privacy policy (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AudioPrivacyPolicy
// Privacy policy for audio fingerprinting. Compile-only skeleton.
struct AudioPrivacyPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool salted_per_install{true};
    bool no_stable_cross_user_fingerprint{true};

    bool operator==(const AudioPrivacyPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::security
