// bitperfect_strict_claim_gate.hpp — P6A capability: strict claim gate (§capability)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::capability {

// BitperfectStrictClaimGate
// Strict gate for bit-perfect claims. all_conditions_met() is a declared
// member function; compile-only skeleton has no runtime definition here.
struct BitperfectStrictClaimGate {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool exclusive_mode_active{false};
    bool exact_endpoint_format{false};
    bool no_mutating_stage{false};
    bool no_resampler{false};
    bool no_dsp{false};
    bool no_drc_dialnorm{false};
    bool no_replaygain{false};
    bool no_software_volume{false};
    bool no_limiter{false};
    bool no_time_stretch{false};
    bool no_channel_remap{false};
    bool no_silence_fill{false};
    bool software_path_verified{false};
    bool endpoint_volume_lab_verified{false};
    bool apo_bypassed{false};
    bool ducking_disabled{false};
    bool freshness_valid{false};
    bool evidence_not_expired{false};

    bool operator==(const BitperfectStrictClaimGate&) const = default;

    bool all_conditions_met() const {
        return exclusive_mode_active && exact_endpoint_format && no_mutating_stage
            && no_resampler && no_dsp && no_drc_dialnorm && no_replaygain
            && no_software_volume && no_limiter && no_time_stretch
            && no_channel_remap && no_silence_fill && software_path_verified
            && endpoint_volume_lab_verified && apo_bypassed && ducking_disabled
            && freshness_valid && evidence_not_expired;
    }
};

}  // namespace kivo::video::audio_plane::capability
