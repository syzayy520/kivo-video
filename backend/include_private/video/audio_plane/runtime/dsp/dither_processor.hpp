// dither_processor.hpp — P6E: dither processor
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::dsp {

enum class DitherKind : int32_t {
    None = 0,
    Triangular = 1,
    Rectangular = 2,
};

// DitherProcessor
// Applies dither noise before quantization to reduce quantization distortion.
class DitherProcessor {
public:
    DitherProcessor() = default;

    void set_kind(DitherKind kind) noexcept { kind_ = kind; }
    DitherKind kind() const noexcept { return kind_; }

    // Apply dither to a float sample, return dithered float sample.
    float process(float sample) noexcept;

private:
    DitherKind kind_{DitherKind::None};
    uint32_t lcg_state_{12345};  // simple LCG for pseudo-random noise

    float next_random() noexcept;
};

}  // namespace kivo::video::audio_plane::backend::runtime::dsp
