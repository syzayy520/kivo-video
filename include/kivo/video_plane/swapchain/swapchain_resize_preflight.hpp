// swapchain_resize_preflight.hpp — GATE-017: P5A swapchain resize preflight
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::swapchain {

struct SwapchainResizePreflight {
    uint32_t target_width{0};               // target width after resize
    uint32_t target_height{0};              // target height after resize
    bool all_views_released{false};         // all backbuffer views released
    bool all_refs_released{false};          // all backbuffer refs released
    bool safe_to_resize{false};             // safe to proceed with resize
    std::string blocking_reason;            // reason if not safe
    bool operator==(SwapchainResizePreflight const&) const = default;
};

}  // namespace kivo::video_plane::swapchain
