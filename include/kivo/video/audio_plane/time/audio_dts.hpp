// audio_dts.hpp — P6A time: AudioDts (decode timestamp value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioDts {
    int64_t value{0};
    bool operator==(const AudioDts&) const = default;
};
}  // namespace kivo::video::audio_plane::time
