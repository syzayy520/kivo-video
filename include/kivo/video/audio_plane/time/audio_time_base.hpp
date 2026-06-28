// audio_time_base.hpp — P6A time: AudioTimeBase (rational time base)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioTimeBase {
    int64_t num{0};
    int64_t den{0};
    bool is_valid() const { return den != 0; }
    bool operator==(const AudioTimeBase&) const = default;
};
}  // namespace kivo::video::audio_plane::time
