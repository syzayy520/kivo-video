// audio_checked_rescale.hpp — P6A time: AudioCheckedRescale (overflow-safe rescale policy)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
#include "kivo/video/audio_plane/time/audio_time_base.hpp"
namespace kivo::video::audio_plane::time {
enum class TimestampOverflow {
    None = 0,
    MultiplyOverflow = 1,
    AddOverflow = 2,
};
struct AudioCheckedRescale {
    ContractMetadata metadata;
    AudioTimeBase from;
    AudioTimeBase to;
    TimestampOverflow overflow{TimestampOverflow::None};
    bool checked_path_fails_safely{true};
    bool no_silent_wrap{true};
    bool no_long_term_float_accumulation{true};
    bool operator==(const AudioCheckedRescale&) const = default;
};
}  // namespace kivo::video::audio_plane::time
