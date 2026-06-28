// audio_recovery_request.hpp — P6A recovery: recovery request (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/recovery/audio_failure_kind.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioRecoveryRequest
// Request to recover from a failure. Compile-only skeleton.
struct AudioRecoveryRequest {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t request_id{0};
    AudioFailureKind kind{AudioFailureKind::DeviceLost};
    uint64_t correlation_id{0};

    bool operator==(const AudioRecoveryRequest&) const = default;
};

}  // namespace kivo::video::audio_plane::recovery
