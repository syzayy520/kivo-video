// linear_resampler.cpp — P6E: linear interpolation resampler implementation
#include "video/audio_plane/runtime/resampler/linear_resampler.hpp"

namespace kivo::video::audio_plane::backend::runtime::resampler {

void LinearResampler::configure(int32_t src_rate, int32_t dst_rate, int32_t channels) {
    src_rate_ = src_rate;
    dst_rate_ = dst_rate;
    channels_ = channels;
    ratio_ = (src_rate > 0) ? static_cast<double>(dst_rate) / src_rate : 1.0;
    position_ = 0.0;
}

int LinearResampler::process(const int16_t* input, int input_frames,
                              int16_t* output, int max_output_frames) {
    if (!input || input_frames <= 0 || !output || max_output_frames <= 0 || channels_ <= 0) {
        return 0;
    }
    if (ratio_ <= 0.0) return 0;

    int output_frames = 0;
    while (output_frames < max_output_frames) {
        // Current source position
        int src_idx = static_cast<int>(position_);
        if (src_idx >= input_frames - 1) break;

        double frac = position_ - src_idx;
        int src_offset = src_idx * channels_;

        for (int c = 0; c < channels_; ++c) {
            int16_t s0 = input[src_offset + c];
            int16_t s1 = input[src_offset + channels_ + c];
            double sample = s0 + (s1 - s0) * frac;
            output[output_frames * channels_ + c] = static_cast<int16_t>(sample + 0.5);
        }

        position_ += 1.0 / ratio_;
        ++output_frames;
    }

    // Reset position for next batch (simplified — no carry-over)
    position_ -= input_frames;

    return output_frames;
}

}  // namespace kivo::video::audio_plane::backend::runtime::resampler
