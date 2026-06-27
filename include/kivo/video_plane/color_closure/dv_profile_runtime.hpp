// dv_profile_runtime.hpp -- P5H H8+H9: DV Profile 5 fail-closed + user message
#pragma once
#include <cstdint>
#include <string>
#include "../dolby_vision/dv_fallback_policy.hpp"

namespace kivo::video_plane::color_closure {

using dolby_vision::DvFallbackMode;
using dolby_vision::DvFallbackPolicy;

enum class DvProfile {
    Profile5,    // IPT-PQ (fail-closed, no safe base layer)
    Profile7,    // compatible with HDR10 base layer
    Profile8,    // compatible with HDR10/HLG base layer
    Unknown
};

struct DvProfileResult {
    DvProfile profile{DvProfile::Unknown};
    DvFallbackPolicy fallback_policy;
    bool has_safe_base_layer{false};
    std::string user_message;
    std::string evidence_id;
};

class FakeDvProfileHandler {
public:
    DvProfileResult evaluate(DvProfile profile, bool base_layer_compatible) {
        DvProfileResult r;
        r.profile = profile;
        switch (profile) {
            case DvProfile::Profile5:
                // Profile 5: IPT-PQ, no safe base layer -> FAIL CLOSED
                r.fallback_policy.mode = DvFallbackMode::FailClosed;
                r.fallback_policy.reason = "profile5_no_safe_base_layer";
                r.fallback_policy.emit_evidence = true;
                r.has_safe_base_layer = false;
                r.user_message = "Dolby Vision Profile 5 is not supported. Playback stopped for safety.";
                r.evidence_id = "dv_profile5_fail_closed";
                break;
            case DvProfile::Profile7:
                // Profile 7: HDR10 base layer compatible
                r.fallback_policy.mode = DvFallbackMode::ToHdr10;
                r.fallback_policy.reason = "profile7_hdr10_base_layer_compatible";
                r.fallback_policy.emit_evidence = true;
                r.has_safe_base_layer = base_layer_compatible;
                r.user_message = "Dolby Vision Profile 7 falling back to HDR10 base layer.";
                r.evidence_id = "dv_profile7_hdr10_fallback";
                break;
            case DvProfile::Profile8:
                // Profile 8: HDR10/HLG compatible
                r.fallback_policy.mode = base_layer_compatible ?
                    DvFallbackMode::ToHdr10 : DvFallbackMode::FailClosed;
                r.fallback_policy.reason = base_layer_compatible ?
                    "profile8_base_layer_compatible" : "profile8_no_safe_base_layer";
                r.has_safe_base_layer = base_layer_compatible;
                r.user_message = base_layer_compatible ?
                    "Dolby Vision Profile 8 falling back to HDR10/HLG." :
                    "Dolby Vision Profile 8 not supported. Playback stopped.";
                r.evidence_id = base_layer_compatible ?
                    "dv_profile8_fallback" : "dv_profile8_fail_closed";
                break;
            default:
                // Unknown profile -> FAIL CLOSED
                r.fallback_policy.mode = DvFallbackMode::FailClosed;
                r.fallback_policy.reason = "unknown_dv_profile";
                r.has_safe_base_layer = false;
                r.user_message = "Unknown Dolby Vision profile. Playback stopped for safety.";
                r.evidence_id = "dv_unknown_fail_closed";
                break;
        }
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
