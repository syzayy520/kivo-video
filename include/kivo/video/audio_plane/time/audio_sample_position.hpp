// audio_sample_position.hpp — P6A time: AudioSamplePosition (value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioSamplePosition {
    uint64_t value{0};
    bool operator==(const AudioSamplePosition&) const = default;
};
}  // namespace kivo::video::audio_plane::time
