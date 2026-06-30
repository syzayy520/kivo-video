#pragma once

#include <cstdint>

namespace kivo::video::playback_graph {

enum class VideoRenderAttachmentState {
    NoSurface,
    SurfaceAttached,
    SurfaceDetached,
    SurfaceReleased
};

struct VideoSurfaceBindingRequest {
    std::uintptr_t native_handle{0};
    std::int32_t viewport_width{0};
    std::int32_t viewport_height{0};
};

struct VideoSurfaceSnapshot {
    bool ready{false};
    std::int32_t width{0};
    std::int32_t height{0};
    double aspect_ratio{0.0};
    VideoRenderAttachmentState render_state{VideoRenderAttachmentState::NoSurface};
    std::uint32_t dropped_frames{0};
};

}  // namespace kivo::video::playback_graph