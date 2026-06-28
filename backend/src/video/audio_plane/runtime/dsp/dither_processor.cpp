// dither_processor.cpp — P6E: dither processor implementation
#include "video/audio_plane/runtime/dsp/dither_processor.hpp"

namespace kivo::video::audio_plane::backend::runtime::dsp {

float DitherProcessor::next_random() noexcept {
    // Simple LCG (Numerical Recipes constants)
    lcg_state_ = lcg_state_ * 1664525u + 1013904223u;
    return static_cast<float>(lcg_state_) / 4294967296.0f;  // [0, 1)
}

float DitherProcessor::process(float sample) noexcept {
    switch (kind_) {
        case DitherKind::None:
            return sample;
        case DitherKind::Rectangular: {
            float noise = next_random() - 0.5f;  // [-0.5, 0.5)
            return sample + noise;
        }
        case DitherKind::Triangular: {
            float n1 = next_random() - 0.5f;
            float n2 = next_random() - 0.5f;
            return sample + (n1 + n2);  // triangular [-1, 1)
        }
        default:
            return sample;
    }
}

}  // namespace kivo::video::audio_plane::backend::runtime::dsp
