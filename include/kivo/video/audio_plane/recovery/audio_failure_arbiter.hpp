// audio_failure_arbiter.hpp — P6A recovery: failure arbiter (§recovery)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::recovery {

// AudioFailureArbiter
// Rules for arbitrating failures into terminal outcomes. Compile-only skeleton.
struct AudioFailureArbiter {
    kivo::video::audio_plane::ContractMetadata metadata{};
    bool one_correlation_id_one_terminal{true};
    bool deferred_lab_not_pass{true};
    bool claim_downgrade_not_recovery_unless_visible{true};

    bool operator==(const AudioFailureArbiter&) const = default;
};

}  // namespace kivo::video::audio_plane::recovery
