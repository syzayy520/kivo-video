// drift_corrector.hpp — P6E: audio clock drift corrector
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::clock {

// DriftCorrector
// Detects and corrects drift between audio clock and reference clock.
class DriftCorrector {
public:
    DriftCorrector() = default;

    void set_reference_ms(int64_t ref_ms) noexcept { reference_ms_ = ref_ms; }
    void set_audio_ms(int64_t audio_ms) noexcept { audio_ms_ = audio_ms; }

    // Compute drift in milliseconds (positive = audio ahead of reference).
    int64_t drift_ms() const noexcept { return audio_ms_ - reference_ms_; }

    // Returns true if drift exceeds threshold.
    bool needs_correction(int64_t threshold_ms) const noexcept {
        int64_t d = drift_ms();
        return (d > threshold_ms || d < -threshold_ms);
    }

    // Suggested resample ratio adjustment (1.0 = no change).
    // If audio is ahead (drift > 0), slow down (ratio < 1.0).
    // If audio is behind (drift < 0), speed up (ratio > 1.0).
    double correction_ratio(int64_t threshold_ms) const noexcept {
        if (!needs_correction(threshold_ms)) return 1.0;
        int64_t d = drift_ms();
        // Simple proportional correction: 1ms drift → 0.01% ratio change
        double adjust = static_cast<double>(d) * 0.0001;
        return 1.0 - adjust;
    }

    int64_t reference_ms() const noexcept { return reference_ms_; }
    int64_t audio_ms() const noexcept { return audio_ms_; }

private:
    int64_t reference_ms_{0};
    int64_t audio_ms_{0};
};

}  // namespace kivo::video::audio_plane::backend::runtime::clock
