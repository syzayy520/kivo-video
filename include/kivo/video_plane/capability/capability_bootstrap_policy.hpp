#pragma once

#include <cstdint>

namespace kivo::video_plane::capability {

struct CapabilityBootstrapPolicy {
    // Exactly ONE component owns bootstrap initialization
    // Registry populated during bootstrap
    // No self-initialization outside bootstrap order
    uint64_t bootstrap_timeout_ms{5000};
    bool require_all_domains_registered{false};  // P5A: false; P5H: true
    bool operator==(CapabilityBootstrapPolicy const&) const = default;
};

}  // namespace kivo::video_plane::capability
