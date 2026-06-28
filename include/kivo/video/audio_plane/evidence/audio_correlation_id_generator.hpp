// audio_correlation_id_generator.hpp — P6A evidence: correlation ID generator (§11.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::evidence {
struct AudioCorrelationIdGenerator {
    ContractMetadata metadata;
    int32_t origin{0};
    int32_t scope{0};
    uint64_t monotonic_counter{0};
    bool operator==(const AudioCorrelationIdGenerator&) const = default;
};
}  // namespace kivo::video::audio_plane::evidence
