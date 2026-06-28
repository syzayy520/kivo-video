// encoded_access_unit_boundary.hpp — P6A passthrough: EncodedAccessUnitBoundary (§8.4)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::passthrough {
enum class EncodedAccessUnitBoundary {
    Start = 0,
    Middle = 1,
    End = 2,
    Complete = 3,
    SplitStart = 4,
    SplitEnd = 5,
};
}  // namespace kivo::video::audio_plane::passthrough
