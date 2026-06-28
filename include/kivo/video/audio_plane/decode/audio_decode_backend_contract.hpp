// audio_decode_backend_contract.hpp — P6A decode: AudioDecodeBackendContract (§7.1)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::decode {
struct AudioDecodeBackendContract {
    ContractMetadata metadata;
    bool supports_drain{true};
    bool supports_flush{true};
    int32_t max_queue_size{16};
    bool operator==(const AudioDecodeBackendContract&) const = default;
};
}  // namespace kivo::video::audio_plane::decode
