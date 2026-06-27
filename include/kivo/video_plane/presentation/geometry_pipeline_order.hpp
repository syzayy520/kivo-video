// geometry_pipeline_order.hpp — GATE-022: P5A geometry pipeline order
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::presentation {

enum class GeometryStage {
    Source,                                 // source geometry
    Crop,                                   // crop stage
    Scale,                                  // scale stage
    Rotate,                                 // rotate stage
    Output                                  // output geometry
};

struct GeometryPipelineOrder {
    GeometryStage current_stage{GeometryStage::Source};
    bool is_ordered{true};                  // geometry stages are in correct order
    std::string order_description;          // description of geometry order
    bool operator==(GeometryPipelineOrder const&) const = default;
};

}  // namespace kivo::video_plane::presentation
