// audio_clock.hpp — P6E: audio clock implementation
#pragma once

#include <cstdint>

namespace kivo::video::audio_plane::backend::runtime::clock {

// AudioClock
// Tracks audio playback position based on samples written to device.
class AudioClock {
public:
    AudioClock() = default;

    void set_sample_rate(int32_t rate) noexcept { sample_rate_ = rate; }
    void reset() noexcept { samples_written_ = 0; samples_played_ = 0; }

    // Called when samples are written to the device buffer.
    void add_written(uint64_t samples) noexcept { samples_written_ += samples; }

    // Called when device reports samples played.
    void update_played(uint64_t samples) noexcept { samples_played_ = samples; }

    // Current audible position in milliseconds.
    int64_t position_ms() const noexcept;

    // Samples buffered but not yet played.
    uint64_t buffered_samples() const noexcept {
        return (samples_written_ > samples_played_) ? (samples_written_ - samples_played_) : 0;
    }

    int32_t sample_rate() const noexcept { return sample_rate_; }
    uint64_t samples_written() const noexcept { return samples_written_; }
    uint64_t samples_played() const noexcept { return samples_played_; }

private:
    int32_t sample_rate_{0};
    uint64_t samples_written_{0};
    uint64_t samples_played_{0};
};

}  // namespace kivo::video::audio_plane::backend::runtime::clock
