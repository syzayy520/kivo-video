// decode_render_sync_mode.hpp — GATE-016: P5A decode render sync mode
#pragma once
#include <string>

namespace kivo::video_plane::d3d11va {

enum class DecodeRenderSyncMode {
    Synchronous,                            // decode and render are synchronous
    Asynchronous,                           // decode and render are asynchronous
    Pipeline                                // pipelined with frame queue
};

struct DecodeRenderSync {
    DecodeRenderSyncMode mode{DecodeRenderSyncMode::Pipeline};
    uint32_t max_pipeline_depth{3};         // max frames in flight
    bool operator==(DecodeRenderSync const&) const = default;
};

}  // namespace kivo::video_plane::d3d11va
