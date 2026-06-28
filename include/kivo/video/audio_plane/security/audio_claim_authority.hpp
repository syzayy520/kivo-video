// audio_claim_authority.hpp — P6A security: claim authority (§security)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::security {

// AudioClaimAuthority
// Authority rules for claim consumption by P8. Compile-only skeleton.
struct AudioClaimAuthority {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool p8_reads_only_capability_registry{true};
    bool unsupported_claim_token_blocked{true};

    bool operator==(const AudioClaimAuthority&) const = default;
};

}  // namespace kivo::video::audio_plane::security
