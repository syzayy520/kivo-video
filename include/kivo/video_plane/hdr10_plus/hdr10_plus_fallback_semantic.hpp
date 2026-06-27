// hdr10_plus_fallback_semantic.hpp — GATE-014: P5A HDR10+ fallback semantic
#pragma once
#include <string>

namespace kivo::video_plane::hdr10_plus {

enum class Hdr10PlusFallbackTarget {
    ToHdr10,                                // fallback to static HDR10
    ToSdr,                                  // fallback to SDR
    FailClosed,                             // fail-closed
    HoldLastSafeFrame                       // hold last safe frame
};

struct Hdr10PlusFallbackSemantic {
    Hdr10PlusFallbackTarget target{Hdr10PlusFallbackTarget::FailClosed};
    std::string reason;                     // reason for fallback
    bool emit_evidence{true};               // emit evidence on fallback
    bool operator==(Hdr10PlusFallbackSemantic const&) const = default;
};

}  // namespace kivo::video_plane::hdr10_plus
