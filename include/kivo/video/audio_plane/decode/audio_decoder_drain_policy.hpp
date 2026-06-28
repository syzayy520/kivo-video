// audio_decoder_drain_policy.hpp — P6A decode: AudioDecoderDrainPolicy (§7.8)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
struct AudioDecoderDrainPolicy {
    ContractMetadata metadata;
    int32_t drain_timeout_ms{3000};
    int32_t normal_init_timeout_ms{3000};
    int32_t live_init_timeout_ms{5000};
    bool protected_init_timeout_policy_defined{true};
    bool emit_drain_complete_evidence{true};
    bool operator==(const AudioDecoderDrainPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
