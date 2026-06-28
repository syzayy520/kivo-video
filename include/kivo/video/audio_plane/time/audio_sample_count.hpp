// audio_sample_count.hpp — P6A time: AudioSampleCount (value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioSampleCount {
    uint64_t value{0};
    bool operator==(const AudioSampleCount&) const = default;
};
}  // namespace kivo::video::audio_plane::time
