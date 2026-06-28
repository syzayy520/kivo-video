// mix_evidence.hpp — P6A mix: mix evidence record (§9.13)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::mix {
struct MixEvidence {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    int32_t kind{0};
    bool operator==(const MixEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::mix
