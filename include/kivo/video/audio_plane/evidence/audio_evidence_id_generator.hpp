// audio_evidence_id_generator.hpp — P6A evidence: evidence ID generator policy (§11.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceIdGenerator {
    ContractMetadata metadata;
    bool atomic_fetch_add_relaxed{true};
    bool zero_reserved_invalid{true};
    bool wrap_triggers_epoch_reset{true};
    bool no_uuid_on_hot_path{true};
    bool operator==(const AudioEvidenceIdGenerator&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
