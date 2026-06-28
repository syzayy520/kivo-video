// audio_evidence_sink_contract.hpp — P6A evidence: sink contract (§11.16)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceSinkContract {
    ContractMetadata metadata;
    bool merges_lanes_by_timestamp{true};
    bool disk_full_degradation{true};
    bool redacted_json_export{true};
    bool operator==(const AudioEvidenceSinkContract&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
