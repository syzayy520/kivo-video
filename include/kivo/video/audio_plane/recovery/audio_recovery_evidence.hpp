// audio_recovery_evidence.hpp — P6A recovery: recovery evidence (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/recovery/audio_failure_kind.hpp"
#include "kivo/video/audio_plane/recovery/audio_recovery_state.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioRecoveryEvidence
// Evidence record of a recovery attempt. Compile-only skeleton.
struct AudioRecoveryEvidence {
    kivo::video::audio_plane::ContractMetadata metadata{};
    uint64_t evidence_id{0};
    AudioFailureKind kind;
    AudioRecoveryState state{AudioRecoveryState::NotStarted};

    bool operator==(const AudioRecoveryEvidence&) const = default;
};

}  // namespace kivo::video::audio_plane::recovery
