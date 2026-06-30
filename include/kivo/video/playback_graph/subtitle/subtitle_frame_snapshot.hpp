#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/subtitle/subtitle_frame_unavailable_reason.hpp"
#include "kivo/video/playback_graph/subtitle/subtitle_snapshot.hpp"

namespace kivo::video::playback_graph {

struct SubtitleFrameSnapshot {
    bool valid{false};
    bool frame_available{false};
    bool runtime_connected{false};
    SubtitleFrameUnavailableReason unavailable_reason{SubtitleFrameUnavailableReason::None};
    std::int64_t media_time_ms{0};
    std::uint64_t track_id{0};
    SubtitleCueSnapshot primary{};
    SubtitleCueSnapshot secondary{};
};

}  // namespace kivo::video::playback_graph