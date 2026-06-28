// quantize_processor.hpp — P6E: quantization processor
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::dsp {

// QuantizeProcessor
// Quantizes float samples to a target bit depth.
class QuantizeProcessor {
public:
    QuantizeProcessor() = default;

    void set_target_bit_depth(int32_t bits) noexcept { target_bits_ = bits; }
    int32_t target_bit_depth() const noexcept { return target_bits_; }

    // Quantize a float sample [-1.0, 1.0] to target bit depth, return as int32.
    int32_t process(float sample) const noexcept;

private:
    int32_t target_bits_{16};
};

}  // namespace kivo::video::audio_plane::backend::runtime::dsp
