// audio_blocking_call_detector.hpp — P6A threading: blocking detector (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioBlockingCallDetector
// Detector for blocking calls on realtime threads. Compile-only skeleton.
struct AudioBlockingCallDetector {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool detect_blocking_io{true};
    bool detect_mutex{true};
    bool detect_condition_variable{true};

    bool operator==(const AudioBlockingCallDetector&) const = default;
};

}  // namespace kivo::video::audio_plane::threading
