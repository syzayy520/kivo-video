// hdr10_fallback.hpp -- P5H H5: HDR10 static fallback
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::color_closure {

enum class Hdr10FallbackAction {
    NoFallback,         // HDR10 can be displayed directly
    FallbackToSdr,      // fallback to SDR via tone mapping
    FallbackToHLG,      // fallback to HLG
    FailClosed          // cannot fallback, fail
};

struct Hdr10FallbackResult {
    Hdr10FallbackAction action{Hdr10FallbackAction::NoFallback};
    bool fallback_succeeded{false};
    std::string reason;
    std::string evidence_id;
};

class FakeHdr10FallbackHandler {
public:
    Hdr10FallbackResult evaluate(bool hdr_display_available, bool tone_map_supported) {
        Hdr10FallbackResult r;
        if (hdr_display_available) {
            r.action = Hdr10FallbackAction::NoFallback;
            r.fallback_succeeded = true;
            r.reason = "hdr10_display_available";
            r.evidence_id = "hdr10_no_fallback_needed";
        } else if (tone_map_supported) {
            r.action = Hdr10FallbackAction::FallbackToSdr;
            r.fallback_succeeded = true;
            r.reason = "hdr_display_unavailable_tone_map_to_sdr";
            r.evidence_id = "hdr10_fallback_to_sdr";
        } else {
            r.action = Hdr10FallbackAction::FailClosed;
            r.fallback_succeeded = false;
            r.reason = "no_hdr_display_no_tone_map";
            r.evidence_id = "hdr10_fail_closed";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
