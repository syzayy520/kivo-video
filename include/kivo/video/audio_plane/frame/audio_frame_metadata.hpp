// audio_frame_metadata.hpp — P6A frame: AudioFrameMetadata (§9.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/frame/audio_frame_id.hpp"
namespace kivo::video::audio_plane::frame {
struct AudioFrameMetadata {
    ContractMetadata metadata;
    AudioFrameId frame_id;
    int64_t pts{0};
    int64_t duration{0};
    bool is_key_frame{false};
    uint64_t pipeline_generation{0};
    bool operator==(const AudioFrameMetadata&) const = default;
};
}  // namespace kivo::video::audio_plane::frame
