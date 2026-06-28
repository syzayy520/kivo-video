// linear_resampler.hpp — P6E: linear interpolation resampler
#pragma once

#include <cstdint>
#include <vector>

namespace kivo::video::audio_plane::backend::runtime::resampler {

// LinearResampler
// Simple linear interpolation resampler for sample rate conversion.
// Does NOT use libswresample — self-implemented.
class LinearResampler {
public:
    LinearResampler() = default;

    // Configure: source_rate → target_rate
    void configure(int32_t src_rate, int32_t dst_rate, int32_t channels);

    // Process input samples → output samples (interleaved int16)
    // Returns number of output frames produced.
    int process(const int16_t* input, int input_frames,
                int16_t* output, int max_output_frames);

    int32_t src_rate() const noexcept { return src_rate_; }
    int32_t dst_rate() const noexcept { return dst_rate_; }
    int32_t channels() const noexcept { return channels_; }

private:
    int32_t src_rate_{0};
    int32_t dst_rate_{0};
    int32_t channels_{0};
    double ratio_{1.0};  // dst/src
    double position_{0.0};  // fractional position in source
};

}  // namespace kivo::video::audio_plane::backend::runtime::resampler
