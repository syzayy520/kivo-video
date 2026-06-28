// compact_critical_failure_counter.hpp — P6A evidence: compact critical failure counter (§11.15)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct CompactCriticalFailureCounter {
    ContractMetadata metadata;
    uint64_t count{0};
    bool is_compact{true};
    bool is_critical{true};
    bool operator==(const CompactCriticalFailureCounter&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
