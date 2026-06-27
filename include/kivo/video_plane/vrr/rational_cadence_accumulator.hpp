// rational_cadence_accumulator.hpp — GATE-018: P5A rational cadence accumulator
#pragma once
#include <cstdint>

namespace kivo::video_plane::vrr {

struct RationalCadenceAccumulator {
    uint32_t numerator{0};                  // cadence numerator
    uint32_t denominator{0};                // cadence denominator
    uint64_t accumulated_error{0};          // accumulated rounding error
    uint64_t frame_count{0};                // frames since last reset
    bool operator==(RationalCadenceAccumulator const&) const = default;
};

}  // namespace kivo::video_plane::vrr
