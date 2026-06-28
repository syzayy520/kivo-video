// audio_evidence_hotpath_submission.hpp — P6A evidence: hot-path submission policy (§11.12)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceHotpathSubmission {
    ContractMetadata metadata;
    bool spsc_lane_per_producer{true};
    bool no_mutex_on_hot_path{true};
    bool no_heap_allocation_on_hot_path{true};
    bool operator==(const AudioEvidenceHotpathSubmission&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
