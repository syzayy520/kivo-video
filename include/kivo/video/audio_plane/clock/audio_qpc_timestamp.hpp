// audio_qpc_timestamp.hpp — P6A clock: AudioQpcTimestamp (value type)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioQpcTimestamp {
    int64_t value{0};
    bool operator==(const AudioQpcTimestamp&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
