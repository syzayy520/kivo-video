// audio_frame_retire_policy.hpp — P6A frame: AudioFrameRetirePolicy (§9.5)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::frame {
struct AudioFrameRetirePolicy {
    ContractMetadata metadata;
    bool retire_on_generation_discard{true};
    bool retire_on_flush{true};
    bool retire_on_drain{true};
    bool heavy_retire_to_non_realtime{true};
    bool operator==(const AudioFrameRetirePolicy&) const = default;
};
}  // namespace kivo::video::audio_plane::frame
