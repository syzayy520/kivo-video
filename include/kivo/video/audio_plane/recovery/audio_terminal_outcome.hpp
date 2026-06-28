// audio_terminal_outcome.hpp — P6A recovery: terminal outcome (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioTerminalOutcome
// Terminal outcome of a recovery attempt. Compile-only enum.
enum class AudioTerminalOutcome {
    Recovered = 0,
    FallbackSharedMode = 1,
    FallbackPCM = 2,
    RebufferStarted = 3,
    TrackDisabled = 4,
    DeviceMuted = 5,
    FailToUpperLayer = 6,
    UserStopped = 7,
    ProcessSafetyExceeded = 8
};

}  // namespace kivo::video::audio_plane::recovery
