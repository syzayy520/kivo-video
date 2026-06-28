// audio_decode_request.hpp — P6A decode: AudioDecodeRequest (§7.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
struct AudioDecodeRequest {
    ContractMetadata metadata;
    uint64_t request_id{0};
    uint64_t packet_handle_id{0};
    int32_t max_frames{0};
    uint64_t pipeline_generation{0};
    bool operator==(const AudioDecodeRequest&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
