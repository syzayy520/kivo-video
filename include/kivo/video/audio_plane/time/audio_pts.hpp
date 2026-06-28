// audio_pts.hpp — P6A time: AudioPts (presentation timestamp value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioPts {
    int64_t value{0};
    bool operator==(const AudioPts&) const = default;
};
}  // namespace kivo::video::audio_plane::time
