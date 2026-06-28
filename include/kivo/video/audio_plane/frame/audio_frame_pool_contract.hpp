// audio_frame_pool_contract.hpp — P6A frame: AudioFramePoolContract (§9.2)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::frame {
struct AudioFramePoolContract {
    ContractMetadata metadata;
    int32_t max_frames{0};
    int64_t max_bytes{0};
    bool pool_backed{true};
    bool noexcept_move{true};
    bool noexcept_destruct{true};
    bool no_heap_free_on_device_thread{true};
    bool operator==(const AudioFramePoolContract&) const = default;
};
}  // namespace kivo::video::audio_plane::frame
