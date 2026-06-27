// hdr10_plus_runtime.hpp -- P5H H6: HDR10+ Detected/OpaquePreserved/DynamicToneMapUnsupported
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::color_closure {

enum class Hdr10PlusProcessingState {
    NotDetected,
    DetectedOnly,                // metadata detected but not processed
    OpaquePreserved,             // metadata preserved opaquely (no dynamic tone map)
    ParsedButNotApplied,         // parsed but tone map not applied
    DynamicToneMapUnsupported,   // dynamic tone map not supported by display
    AppliedExperimental,         // experimental tone map applied
    AppliedVerified              // verified tone map applied
};

struct Hdr10PlusProcessingResult {
    Hdr10PlusProcessingState state{Hdr10PlusProcessingState::NotDetected};
    bool is_safe{false};          // safe = no wrong color
    std::string reason;
    std::string evidence_id;
};

class FakeHdr10PlusProcessor {
public:
    Hdr10PlusProcessingResult process(bool has_dynamic_metadata,
                                       bool display_supports_dynamic_tone_map,
                                       bool experimental_enabled) {
        Hdr10PlusProcessingResult r;
        if (!has_dynamic_metadata) {
            r.state = Hdr10PlusProcessingState::NotDetected;
            r.is_safe = true;
            r.reason = "no_dynamic_metadata";
            r.evidence_id = "hdr10_plus_not_detected";
            return r;
        }
        if (display_supports_dynamic_tone_map) {
            r.state = experimental_enabled ?
                Hdr10PlusProcessingState::AppliedExperimental :
                Hdr10PlusProcessingState::AppliedVerified;
            r.is_safe = true;
            r.reason = "dynamic_tone_map_applied";
            r.evidence_id = "hdr10_plus_applied";
        } else {
            // Display doesn't support dynamic tone map
            r.state = Hdr10PlusProcessingState::DynamicToneMapUnsupported;
            r.is_safe = true; // safe = preserve opaquely, don't wrong-color
            r.reason = "dynamic_tone_map_unsupported_opaque_preserve";
            r.evidence_id = "hdr10_plus_opaque_preserve";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
