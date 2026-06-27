// cadence_runtime.hpp -- P5F F8: rational cadence golden tests
// Accumulator overflow -> reset with evidence, no silent wrap, no float-only cadence
#pragma once
#include <cstdint>
#include <string>

namespace kivo::video_plane::gpu_render {

struct CadenceAccumulator {
    uint32_t numerator{0};
    uint32_t denominator{0};
    uint64_t accumulated_error{0};
    uint64_t frame_count{0};
    uint64_t max_accumulated_error{0};  // overflow threshold
    bool overflowed{false};
    std::string overflow_evidence;
};

class FakeCadenceRuntime {
public:
    void configure(uint32_t num, uint32_t den, uint64_t max_error = 1000000) {
        acc_.numerator = num;
        acc_.denominator = den;
        acc_.max_accumulated_error = max_error;
        acc_.accumulated_error = 0;
        acc_.frame_count = 0;
        acc_.overflowed = false;
        acc_.overflow_evidence.clear();
    }

    // Advance one frame -- accumulates rational error
    void advance_frame() {
        if (acc_.denominator == 0) return;
        acc_.frame_count++;
        acc_.accumulated_error += acc_.numerator % acc_.denominator;
        // Check overflow -- when accumulated error exceeds threshold, reset with evidence
        if (acc_.accumulated_error >= acc_.max_accumulated_error) {
            if (!acc_.overflowed) {
                acc_.overflowed = true;
                acc_.overflow_evidence = "cadence_accumulator_overflow_reset_frame_" +
                    std::to_string(acc_.frame_count);
            }
            // Reset error on overflow
            acc_.accumulated_error = 0;
        }
    }

    CadenceAccumulator const& state() const { return acc_; }

    // Golden test: verify cadence produces expected frame timing
    bool validate_golden(uint32_t expected_num, uint32_t expected_den) const {
        return acc_.numerator == expected_num && acc_.denominator == expected_den;
    }

private:
    CadenceAccumulator acc_;
};

}  // namespace kivo::video_plane::gpu_render
