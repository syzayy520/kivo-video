// backbuffer_reference_tracker.hpp — GATE-017: P5A backbuffer reference tracker
#pragma once
#include <cstdint>

namespace kivo::video_plane::swapchain {

struct BackbufferReferenceTracker {
    uint64_t backbuffer_handle{0};          // opaque backbuffer handle
    uint32_t ref_count{0};                  // current reference count
    uint32_t max_refs{8};                   // max references allowed
    bool is_released{false};                // backbuffer has been released
    bool operator==(BackbufferReferenceTracker const&) const = default;
};

}  // namespace kivo::video_plane::swapchain
