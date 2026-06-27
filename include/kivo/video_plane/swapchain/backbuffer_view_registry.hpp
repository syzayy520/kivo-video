// backbuffer_view_registry.hpp — GATE-017: P5A backbuffer view registry
#pragma once
#include <cstdint>
#include <vector>

namespace kivo::video_plane::swapchain {

struct BackbufferViewRegistry {
    std::vector<uint64_t> registered_views; // registered view handles
    uint32_t view_count{0};                 // current view count
    uint32_t max_views{16};                 // max views per backbuffer
    bool operator==(BackbufferViewRegistry const&) const = default;
};

}  // namespace kivo::video_plane::swapchain
