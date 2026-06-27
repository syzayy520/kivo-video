// deferred_command_list_tracker.hpp — GATE-017: P5A deferred command list tracker
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::swapchain {

struct DeferredCommandListTracker {
    uint64_t command_list_handle{0};        // opaque command list handle
    bool is_deferred{false};                // command list is deferred
    uint32_t pending_command_count{0};      // pending commands
    std::string state;                      // "recording", "closed", "executed"
    bool operator==(DeferredCommandListTracker const&) const = default;
};

}  // namespace kivo::video_plane::swapchain
