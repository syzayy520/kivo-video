#pragma once

#include <string>

namespace kivo::video_plane::capability {

struct CapabilityRuntimeState {
    bool active{false};
    bool degraded{false};
    std::string degradation_reason;
    bool operator==(CapabilityRuntimeState const&) const = default;
};

}  // namespace kivo::video_plane::capability
