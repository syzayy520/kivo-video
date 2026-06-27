// video_pipeline_id.hpp — GATE-010: P5A video pipeline id
#pragma once
#include <cstdint>

namespace kivo::video_plane::queue {

struct VideoPipelineId {
    uint64_t value{0};
    bool operator==(VideoPipelineId const&) const = default;
};

}  // namespace kivo::video_plane::queue
