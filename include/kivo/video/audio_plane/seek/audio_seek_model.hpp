// audio_seek_model.hpp — P6A seek: audio seek model contract (§6)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::seek {
struct AudioSeekModel {
    kivo::video::audio_plane::ContractMetadata metadata;
    int64_t target_media_time_ms{0};
    int32_t precision{0};
    bool recovery_seek{false};
};
}  // namespace kivo::video::audio_plane::seek
