// render_thread_destructor_policy.hpp — P5D D5: No heavy destructor on RenderThread
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::decoder {

struct DestructorPolicy {
    bool defer_heavy_destructors{true};
    uint64_t max_render_thread_destructor_ms{1};  // max 1ms on render thread
    std::string deferred_destructor_thread{"cleanup_thread"};
    bool operator==(DestructorPolicy const&) const = default;
};

// Check if a destructor is too heavy for render thread
inline bool is_destructor_too_heavy(
    DestructorPolicy const& policy,
    uint64_t destructor_duration_ms) {
    if (!policy.defer_heavy_destructors) return false;
    return destructor_duration_ms > policy.max_render_thread_destructor_ms;
}

}  // namespace kivo::video_plane::decoder
