// audio_evidence_ttl_policy.hpp — P6A evidence: TTL policy (§11.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioEvidenceTtlPolicy {
    ContractMetadata metadata;
    bool device_format_valid_until_device_gen_change{true};
    bool ducking_valid_until_ducking_gen_change{true};
    bool lab_volume_tied_to_device_fingerprint{true};
    bool bit_perfect_valid_only_for_freshness_matrix{true};
    bool operator==(const AudioEvidenceTtlPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
