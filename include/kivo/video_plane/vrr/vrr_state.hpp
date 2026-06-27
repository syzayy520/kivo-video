// vrr_state.hpp — GATE-018: P5A VRR state
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::vrr {

enum class VrrAvailability {
    NotChecked,                             // not yet checked
    Available,                              // VRR available
    NotAvailable,                           // VRR not available
    Disabled                                // VRR available but disabled
};

struct VrrState {
    VrrAvailability availability{VrrAvailability::NotChecked};
    uint32_t min_refresh_rate{0};           // min VRR refresh rate (Hz)
    uint32_t max_refresh_rate{0};           // max VRR refresh rate (Hz)
    bool operator==(VrrState const&) const = default;
};

}  // namespace kivo::video_plane::vrr
