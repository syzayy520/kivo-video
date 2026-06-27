// hold_last_frame_safe.hpp — GATE-022: P5A hold last frame safe
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::presentation {

struct HoldLastFrameSafe {
    uint64_t frame_index{0};                // last safe frame index
    bool is_holding{false};                 // currently holding last safe frame
    std::string reason;                     // reason for holding
    uint64_t hold_started_timestamp{0};     // when hold started
    bool operator==(HoldLastFrameSafe const&) const = default;
};

}  // namespace kivo::video_plane::presentation
