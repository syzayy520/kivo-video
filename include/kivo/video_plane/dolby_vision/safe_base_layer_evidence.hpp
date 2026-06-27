// safe_base_layer_evidence.hpp — GATE-013: P5A Dolby Vision safe base layer evidence
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::dolby_vision {

struct SafeBaseLayerEvidence {
    uint64_t frame_index{0};                // frame that used safe base layer
    std::string reason;                     // why base layer fallback was used
    bool bl_present{false};                 // base layer present
    bool el_present{false};                 // enhancement layer present
    std::string evidence_id;
    bool operator==(SafeBaseLayerEvidence const&) const = default;
};

}  // namespace kivo::video_plane::dolby_vision
