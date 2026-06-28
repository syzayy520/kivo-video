// audio_failure_kind.hpp — P6A recovery: failure kind (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioFailureKind
// Kind of audio failure. Compile-only enum.
enum class AudioFailureKind {
    DeviceLost = 0,
    FormatNotSupported = 1,
    DecodeFailed = 2,
    Underrun = 3,
    Timeout = 4,
    Superseded = 5,
    ProcessSafety = 6
};

}  // namespace kivo::video::audio_plane::recovery
