// audio_channel_evidence.hpp — P6A channel: audio channel evidence (§8.8)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::channel {
struct AudioChannelEvidence {
    ContractMetadata metadata;
    uint64_t evidence_id{0};
    int32_t kind{0};
    bool operator==(const AudioChannelEvidence&) const = default;
};
}  // namespace kivo::video::audio_plane::channel
