// dv_fallback_policy.hpp — GATE-013: P5A Dolby Vision fallback policy
#pragma once
#include <string>

namespace kivo::video_plane::dolby_vision {

enum class DvFallbackMode {
    ToHdr10,                                // fallback to HDR10
    ToSdr,                                  // fallback to SDR
    FailClosed,                             // fail-closed, no playback
    HoldLastSafeFrame                       // hold last safe frame
};

struct DvFallbackPolicy {
    DvFallbackMode mode{DvFallbackMode::FailClosed};
    std::string reason;                     // reason for fallback
    bool emit_evidence{true};               // emit evidence on fallback
    bool operator==(DvFallbackPolicy const&) const = default;
};

}  // namespace kivo::video_plane::dolby_vision
