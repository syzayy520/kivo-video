// rotation_viewport.hpp -- P5F F9: rotation 90/270 viewport recompute
#pragma once
#include <cstdint>

namespace kivo::video_plane::gpu_render {

enum class Rotation {
    None,       // 0 degrees
    Rotate90,   // 90 degrees clockwise
    Rotate180,  // 180 degrees
    Rotate270   // 270 degrees clockwise
};

struct Viewport {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};
};

// Recompute viewport for rotation: 90/270 swap width and height
inline Viewport recompute_viewport(Viewport const& original, Rotation rotation) {
    Viewport result = original;
    if (rotation == Rotation::Rotate90 || rotation == Rotation::Rotate270) {
        // Swap width and height
        result.width = original.height;
        result.height = original.width;
        // Recompute origin
        if (rotation == Rotation::Rotate90) {
            result.x = original.y;
            result.y = static_cast<int32_t>(original.width) - original.x - static_cast<int32_t>(original.height);
        } else { // Rotate270
            result.x = static_cast<int32_t>(original.height) - original.y - static_cast<int32_t>(original.width);
            result.y = original.x;
        }
    } else if (rotation == Rotation::Rotate180) {
        result.x = static_cast<int32_t>(original.width) - original.x;
        result.y = static_cast<int32_t>(original.height) - original.y;
    }
    return result;
}

}  // namespace kivo::video_plane::gpu_render
