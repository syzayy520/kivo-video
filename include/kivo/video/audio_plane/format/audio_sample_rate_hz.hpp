// audio_sample_rate_hz.hpp — P6A format: audio sample rate hz value type (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::format {
struct AudioSampleRateHz {
    uint32_t value{0};
    bool is_valid() const { return value > 0; }
    bool operator==(const AudioSampleRateHz&) const = default;
};
}  // namespace kivo::video::audio_plane::format
