// opaque_fallback_policy.hpp — GATE-016: P5A opaque fallback policy
#pragma once
#include <string>

namespace kivo::video_plane::d3d11va {

enum class OpaqueFallbackTarget {
    SoftwareDecode,                         // fallback to software decode
    HoldLastSafeFrame,                      // hold last safe frame
    FailToUpperLayer,                       // fail to upper layer
    Retry                                   // retry hardware decode
};

struct OpaqueFallbackPolicy {
    OpaqueFallbackTarget target{OpaqueFallbackTarget::SoftwareDecode};
    std::string reason;                     // reason for fallback
    uint32_t max_retries{3};                // max retries before fail
    bool operator==(OpaqueFallbackPolicy const&) const = default;
};

}  // namespace kivo::video_plane::d3d11va
