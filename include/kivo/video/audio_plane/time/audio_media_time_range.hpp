// audio_media_time_range.hpp — P6A time: AudioMediaTimeRange (closed range)
#pragma once
#include <cstdint>
#include "kivo/video/audio_plane/foundation/contract_metadata.hpp"
namespace kivo::video::audio_plane::time {
struct AudioMediaTimeRange {
    int64_t start{0};
    int64_t end{0};
    bool is_valid() const { return end >= start; }
    bool operator==(const AudioMediaTimeRange&) const = default;
};
}  // namespace kivo::video::audio_plane::time
