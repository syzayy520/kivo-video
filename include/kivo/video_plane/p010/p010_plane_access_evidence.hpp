// p010_plane_access_evidence.hpp — GATE-015: P5A P010 plane access evidence
#pragma once
#include <cstdint>
#include <string>
#include "p010_formula.hpp"

namespace kivo::video_plane::p010 {

struct P010PlaneAccessEvidence {
    P010Formula formula;
    uint64_t frame_index{0};                // frame that was accessed
    std::string access_result;              // "success" or failure description
    std::string evidence_id;
    uint64_t timestamp{0};
    bool operator==(P010PlaneAccessEvidence const&) const = default;
};

}  // namespace kivo::video_plane::p010
