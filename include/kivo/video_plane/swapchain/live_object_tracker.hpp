// live_object_tracker.hpp — GATE-017: P5A live object tracker
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace kivo::video_plane::swapchain {

struct LiveObjectTracker {
    uint32_t live_object_count{0};          // current live object count
    uint32_t max_live_objects{256};         // max allowed live objects
    std::vector<std::string> object_types;  // types of live objects
    bool leak_detected{false};              // potential leak detected
    bool operator==(LiveObjectTracker const&) const = default;
};

}  // namespace kivo::video_plane::swapchain
