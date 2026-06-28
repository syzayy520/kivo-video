// audio_output_flush_request.hpp — P6A §5: flush request
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::output {
enum class FlushScope : int32_t { Render = 0, Decode = 1, Full = 2 };
struct AudioOutputFlushRequest {
    ContractMetadata metadata;
    FlushScope scope{FlushScope::Full};
    uint64_t pipeline_generation{0};
    bool discard_stale_by_generation{true};
};
}  // namespace kivo::video::audio_plane::output
