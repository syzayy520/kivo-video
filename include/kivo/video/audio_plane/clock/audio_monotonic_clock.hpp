// audio_monotonic_clock.hpp — P6A clock: AudioMonotonicClock
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
struct AudioMonotonicClock {
    ContractMetadata metadata;
    int64_t now_ms{0};
    bool monotonic_guarantee{true};
    bool operator==(const AudioMonotonicClock&) const = default;
};
}  // namespace kivo::video::audio_plane::clock
