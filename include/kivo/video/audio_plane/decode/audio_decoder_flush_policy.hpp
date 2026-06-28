// audio_decoder_flush_policy.hpp — P6A decode: AudioDecoderFlushPolicy (§7.7)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
struct AudioDecoderFlushPolicy {
    ContractMetadata metadata;
    bool cancel_pending_decode{true};
    bool release_all_p6_handles{true};
    int32_t flush_timeout_ms{1000};
    bool operator==(const AudioDecoderFlushPolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
