// hlg_fallback.hpp -- P5H H7: HLG SDR fallback
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::color_closure {

enum class HlgFallbackAction {
    NoFallback,          // HLG can be displayed directly
    FallbackToSdr,       // fallback to SDR via HLG->SDR conversion
    FallbackToHdr10,     // fallback to HDR10 static metadata
    FailClosed           // cannot fallback
};

struct HlgFallbackResult {
    HlgFallbackAction action{HlgFallbackAction::NoFallback};
    bool fallback_succeeded{false};
    std::string reason;
    std::string evidence_id;
};

class FakeHlgFallbackHandler {
public:
    HlgFallbackResult evaluate(bool hlg_display_available, bool sdr_conversion_supported) {
        HlgFallbackResult r;
        if (hlg_display_available) {
            r.action = HlgFallbackAction::NoFallback;
            r.fallback_succeeded = true;
            r.reason = "hlg_display_available";
            r.evidence_id = "hlg_no_fallback";
        } else if (sdr_conversion_supported) {
            r.action = HlgFallbackAction::FallbackToSdr;
            r.fallback_succeeded = true;
            r.reason = "hlg_to_sdr_conversion";
            r.evidence_id = "hlg_fallback_to_sdr";
        } else {
            r.action = HlgFallbackAction::FailClosed;
            r.fallback_succeeded = false;
            r.reason = "no_hlg_display_no_sdr_conversion";
            r.evidence_id = "hlg_fail_closed";
        }
        return r;
    }
};

}  // namespace kivo::video_plane::color_closure
