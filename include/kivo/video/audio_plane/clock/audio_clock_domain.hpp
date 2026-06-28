// audio_clock_domain.hpp — P6A clock: AudioClockDomain (enum)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
enum class AudioClockDomain {
    OutputDevice = 0,
    RenderCursor = 1,
    P4Synthetic = 2,
    P4VideoProxy = 3,
    ExternalLive = 4,
    Monotonic = 5,
};
}  // namespace kivo::video::audio_plane::clock
