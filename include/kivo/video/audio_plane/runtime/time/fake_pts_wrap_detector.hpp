// fake_pts_wrap_detector.hpp — P6B runtime: fake PTS wrap/jump detector
// Header-only fake implementation. Platform-neutral.
#pragma once

#include <cstdint>
#include "kivo/video/audio_plane/time/audio_pts.hpp"

namespace kivo::video::audio_plane::runtime::time {

// Wrap status reported by the detector.
enum class WrapStatus : int32_t {
    NoWrap = 0,         // current >= prev (normal forward progression)
    ForwardWrap = 1,    // current < prev AND delta exceeds wrap threshold
    BackwardJump = 2,   // current < prev AND delta within tolerance (seek-back)
    Impossible = 3,     // current > prev AND forward jump exceeds max (corrupt)
};

// Fake in-memory PTS wrap detector.
// Thresholds are expressed in timebase units (caller decides the timebase).
class FakePtsWrapDetector {
public:
    FakePtsWrapDetector() = default;

    // Configure thresholds. Defaults assume a 90 kHz timebase:
    //   wrap_threshold  = 1 << 33 (~2^33 ticks, ~26.4h at 90kHz)
    //   backward_tolerance = 1 << 20  (~3.7s at 90kHz, treats small seeks as BackwardJump)
    //   max_forward_jump = 5s in 90kHz units = 450000
    void configure(
        int64_t wrap_threshold,
        int64_t backward_tolerance,
        int64_t max_forward_jump) noexcept {
        wrap_threshold_ = wrap_threshold;
        backward_tolerance_ = backward_tolerance;
        max_forward_jump_ = max_forward_jump;
    }

    // Detect wrap/jump between prev and current. If prev has not been set
    // (first sample), returns NoWrap and seeds prev.
    WrapStatus detect(int64_t prev, int64_t current) noexcept {
        if (!has_prev_) {
            has_prev_ = true;
            prev_ = current;
            return WrapStatus::NoWrap;
        }
        WrapStatus status = classify(prev, current);
        prev_ = current;
        return status;
    }

    // Clear prev, e.g. after seek/flush/discontinuity. Next detect() seeds.
    void reset() noexcept {
        has_prev_ = false;
        prev_ = 0;
    }

private:
    WrapStatus classify(int64_t prev, int64_t current) const noexcept {
        if (current >= prev) {
            int64_t forward_delta = current - prev;
            if (forward_delta > max_forward_jump_) {
                return WrapStatus::Impossible;
            }
            return WrapStatus::NoWrap;
        }
        // current < prev: either wrap or backward jump.
        int64_t backward_delta = prev - current;
        if (backward_delta > wrap_threshold_) {
            return WrapStatus::ForwardWrap;
        }
        if (backward_delta <= backward_tolerance_) {
            return WrapStatus::BackwardJump;
        }
        // Large backward delta but below wrap threshold: treat as BackwardJump
        // (long seek-back). Callers may upgrade via their own policy.
        return WrapStatus::BackwardJump;
    }

    bool has_prev_{false};
    int64_t prev_{0};
    int64_t wrap_threshold_{1LL << 33};
    int64_t backward_tolerance_{1LL << 20};
    int64_t max_forward_jump_{450000};
};

}  // namespace kivo::video::audio_plane::runtime::time
