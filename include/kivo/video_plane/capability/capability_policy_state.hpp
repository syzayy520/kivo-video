#pragma once

#include <string>

namespace kivo::video_plane::capability {

struct CapabilityPolicyState {
    bool enabled_by_policy{false};
    bool restricted_by_policy{false};
    std::string policy_id;
    std::string policy_reason;
    bool operator==(CapabilityPolicyState const&) const = default;
};

}  // namespace kivo::video_plane::capability
