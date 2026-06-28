// audio_clock.cpp — P6E: audio clock implementation
#include "video/audio_plane/runtime/clock/audio_clock.hpp"

namespace kivo::video::audio_plane::backend::runtime::clock {

int64_t AudioClock::position_ms() const noexcept {
    if (sample_rate_ <= 0) return 0;
    // Audible position = samples played by device
    return static_cast<int64_t>((samples_played_ * 1000) / sample_rate_);
}

}  // namespace kivo::video::audio_plane::backend::runtime::clock
