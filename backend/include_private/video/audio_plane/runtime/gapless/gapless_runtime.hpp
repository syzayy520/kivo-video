// gapless_runtime.hpp — P6E: gapless playback runtime
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::gapless {

// GaplessRuntime
// Manages gapless track transitions: codec delay trim, encoder padding trim,
// and cross-track sample continuity.
class GaplessRuntime {
public:
    GaplessRuntime() = default;

    // Set codec delay (samples to skip at start of track).
    void set_codec_delay_samples(uint32_t samples) noexcept { codec_delay_ = samples; }

    // Set encoder padding (samples to skip at end of track).
    void set_encoder_padding_samples(uint32_t samples) noexcept { encoder_padding_ = samples; }

    // Set total track samples (before trim).
    void set_total_samples(uint64_t samples) noexcept { total_samples_ = samples; }

    // Compute the audible sample range after trim.
    uint64_t audible_start_sample() const noexcept { return codec_delay_; }
    uint64_t audible_end_sample() const noexcept {
        return (total_samples_ > encoder_padding_) ? (total_samples_ - encoder_padding_) : 0;
    }
    uint64_t audible_sample_count() const noexcept {
        uint64_t end = audible_end_sample();
        return (end > codec_delay_) ? (end - codec_delay_) : 0;
    }

    // Check if a given sample position is audible (not in delay or padding).
    bool is_audible(uint64_t sample_position) const noexcept {
        return sample_position >= codec_delay_ && sample_position < audible_end_sample();
    }

    uint32_t codec_delay() const noexcept { return codec_delay_; }
    uint32_t encoder_padding() const noexcept { return encoder_padding_; }
    uint64_t total_samples() const noexcept { return total_samples_; }

private:
    uint32_t codec_delay_{0};
    uint32_t encoder_padding_{0};
    uint64_t total_samples_{0};
};

}  // namespace kivo::video::audio_plane::backend::runtime::gapless
