// audio_sample_duration.hpp — P6A time: AudioSampleDuration (value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioSampleDuration {
    uint64_t value{0};
    bool operator==(const AudioSampleDuration&) const = default;
};
}  // namespace kivo::video::audio_plane::time
