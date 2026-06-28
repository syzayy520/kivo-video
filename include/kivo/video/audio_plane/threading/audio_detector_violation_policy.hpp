// audio_detector_violation_policy.hpp — P6A threading: detector violation (§threading)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::threading {

// AudioDetectorViolationPolicy
// Policy for handling detector violations. Compile-only skeleton.
struct AudioDetectorViolationPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool emit_evidence{true};
    bool increment_critical_counter{true};
    bool set_async_recovery_flag{true};

    bool operator==(const AudioDetectorViolationPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::threading
