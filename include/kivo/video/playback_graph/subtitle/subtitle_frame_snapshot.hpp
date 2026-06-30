#pragma once

#include "kivo/video/playback_graph/subtitle/subtitle_snapshot.hpp"

namespace kivo::video::playback_graph {

struct SubtitleFrameSnapshot {
    bool frame_available{false};
    SubtitleCueSnapshot primary{};
    SubtitleCueSnapshot secondary{};
    bool valid{false};
};

}  // namespace kivo::video::playback_graph