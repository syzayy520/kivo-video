// decoded_audio_frame.hpp — P6A decode: DecodedAudioFrame (§7.6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
struct DecodedAudioFrame {
    ContractMetadata metadata;
    uint64_t frame_id{0};
    int32_t sample_rate_hz{0};
    int32_t channel_count{0};
    int32_t bit_depth{0};
    int64_t pts{0};
    int64_t duration{0};
    uint64_t pool_slot_id{0};
    bool is_key_frame{false};
    bool operator==(const DecodedAudioFrame&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
