// checked_rescale.hpp — GATE-020: P5A checked rescale
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::timebase {

struct CheckedRescale {
    uint64_t input_num{0};                  // input numerator
    uint64_t input_den{1};                  // input denominator
    uint64_t output_num{0};                 // output numerator
    uint64_t output_den{1};                 // output denominator
    bool overflow_detected{false};          // overflow detected during rescale
    std::string overflow_reason;            // reason if overflow
    bool operator==(CheckedRescale const&) const = default;
};

}  // namespace kivo::video_plane::timebase
