// audio_clock_candidate.hpp — P6A clock: AudioClockCandidate (enum)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::clock {
enum class AudioClockCandidate {
    OutputDeviceClock = 0,
    AudioRenderCursor = 1,
    P4SyntheticTimeline = 2,
    P4VideoTimelineProxy = 3,
    ExternalLiveClock = 4,
    MonotonicFallback = 5,
};
}  // namespace kivo::video::audio_plane::clock
