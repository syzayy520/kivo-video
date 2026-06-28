// quantize_processor.cpp — P6E: quantization processor implementation
#include "video/audio_plane/runtime/dsp/quantize_processor.hpp"
#include <cmath>

namespace kivo::video::audio_plane::backend::runtime::dsp {

int32_t QuantizeProcessor::process(float sample) const noexcept {
    if (target_bits_ <= 0 || target_bits_ > 32) return 0;
    int32_t max_val = (1 << (target_bits_ - 1)) - 1;   // 32767 for 16-bit
    int32_t min_val = -(1 << (target_bits_ - 1));        // -32768 for 16-bit
    // Clamp to [-1.0, 1.0]
    if (sample > 1.0f) sample = 1.0f;
    if (sample < -1.0f) sample = -1.0f;
    // Scale: positive uses max_val, negative uses min_val (asymmetric, standard PCM)
    double scaled;
    if (sample >= 0.0f) {
        scaled = static_cast<double>(sample) * max_val;
    } else {
        scaled = static_cast<double>(sample) * (-min_val);  // -1.0 * 32768 = -32768
    }
    int32_t result = static_cast<int32_t>(std::round(scaled));
    if (result > max_val) result = max_val;
    if (result < min_val) result = min_val;
    return result;
}

}  // namespace kivo::video::audio_plane::backend::runtime::dsp
