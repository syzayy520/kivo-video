// display_capability_snapshot.hpp — GATE-024: P5A display capability snapshot
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu {

struct DisplayCapabilitySnapshot {
    uint64_t display_id{0};                 // display identifier
    std::string display_name;               // display name
    uint32_t max_refresh_rate{0};           // max refresh rate (Hz)
    uint32_t current_refresh_rate{0};       // current refresh rate (Hz)
    bool hdr_capable{false};                // display is HDR capable
    bool vrr_capable{false};                // display is VRR capable
    bool operator==(DisplayCapabilitySnapshot const&) const = default;
};

}  // namespace kivo::video_plane::gpu
