// audio_frame_buffer.hpp — P6A frame: AudioFrameBuffer (§9.3)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::frame {
struct AudioFrameBuffer {
    ContractMetadata metadata;
    uint64_t pool_slot_id{0};
    int32_t channel_count{0};
    int32_t sample_count{0};
    int32_t bit_depth{0};
    bool is_planar{false};
    bool operator==(const AudioFrameBuffer&) const = default;
};
}  // namespace kivo::video::audio_plane::frame
