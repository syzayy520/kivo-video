// noise_shaping_processor.hpp — P6E: noise shaping processor
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::dsp {

// NoiseShapingProcessor
// First-order noise shaping: feeds quantization error back to next sample.
class NoiseShapingProcessor {
public:
    NoiseShapingProcessor() = default;

    void reset() noexcept { error_ = 0.0f; }

    // Process a float sample with noise shaping.
    // Returns the quantization error for the next iteration.
    float process(float sample, float quantized) noexcept {
        error_ = (sample - quantized) * 0.5f;  // first-order feedback
        return error_;
    }

    // Apply noise shaping to a sample before quantization.
    float apply_feedback(float sample) const noexcept {
        return sample + error_;
    }

    float current_error() const noexcept { return error_; }

private:
    float error_{0.0f};
};

}  // namespace kivo::video::audio_plane::backend::runtime::dsp
