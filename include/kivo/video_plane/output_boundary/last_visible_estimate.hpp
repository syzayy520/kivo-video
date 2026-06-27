// last_visible_estimate.hpp — GATE-023: P5A last visible estimate
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::output_boundary {

struct LastVisibleEstimate {
    uint64_t frame_index{0};                // last visible frame index
    uint64_t media_timestamp{0};            // last visible media timestamp
    uint64_t estimate_timestamp{0};         // when estimate was made
    bool is_confident{false};               // estimate is confident
    bool operator==(LastVisibleEstimate const&) const = default;
};

}  // namespace kivo::video_plane::output_boundary
