// audio_recovery_policy.hpp — P6A recovery: recovery policy (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioRecoveryPolicy
// Policy for recovery attempts. Compile-only skeleton.
struct AudioRecoveryPolicy {
    kivo::video::audio_plane::ContractMetadata metadata{};
    int32_t max_attempts{3};
    int32_t timeout_ms{2000};
    bool request_recovery_seek{true};

    bool operator==(const AudioRecoveryPolicy&) const = default;
};

}  // namespace kivo::video::audio_plane::recovery
