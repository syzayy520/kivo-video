// protected_placeholder_frame.hpp — GATE-011: P5A protected placeholder frame
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::protected_content {

struct ProtectedPlaceholderFrame {
    uint64_t frame_index{0};                // which frame is placeholder
    std::string reason;                     // why placeholder was used
    bool is_black{true};                    // placeholder is black frame
    bool is_last_safe_frame{false};         // placeholder is last safe frame
    bool operator==(ProtectedPlaceholderFrame const&) const = default;
};

}  // namespace kivo::video_plane::protected_content
