#include "video/video_plane/probe/audio/aaudio_tone_source.hpp"

#include <cmath>
#include <cstddef>

namespace kivo::video::video_plane::probe {
namespace {

constexpr double kTwoPi = 6.283185307179586476925286766559;
constexpr double kDefaultSampleRate = 48000.0;

}  // namespace

AaudioToneSource::AaudioToneSource(std::int32_t sample_rate,
                                   std::int32_t channel_count,
                                   double frequency_hz,
                                   double amplitude)
    : channel_count_(channel_count > 0 ? channel_count : 1),
      amplitude_(amplitude),
      phase_(0.0),
      phase_increment_(kTwoPi * frequency_hz /
                       (sample_rate > 0 ? static_cast<double>(sample_rate) : kDefaultSampleRate)) {}

std::int32_t AaudioToneSource::render(float* interleaved, std::int32_t frame_count) {
    if (interleaved == nullptr || frame_count <= 0) {
        return 0;
    }

    for (std::int32_t frame = 0; frame < frame_count; ++frame) {
        const auto sample = static_cast<float>(amplitude_ * std::sin(phase_));
        const auto base = static_cast<std::size_t>(frame) * static_cast<std::size_t>(channel_count_);
        for (std::int32_t channel = 0; channel < channel_count_; ++channel) {
            interleaved[base + static_cast<std::size_t>(channel)] = sample;
        }

        phase_ += phase_increment_;
        if (phase_ >= kTwoPi) {
            phase_ -= kTwoPi;
        }
    }

    return frame_count;
}

}  // namespace kivo::video::video_plane::probe
