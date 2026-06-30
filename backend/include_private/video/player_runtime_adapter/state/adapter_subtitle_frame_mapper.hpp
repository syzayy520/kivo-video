#pragma once

#include "kivo/video/playback_graph/subtitle/subtitle_frame_snapshot.hpp"
#include "kivo/video/player_runtime_adapter/adapter_snapshot.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

void apply_subtitle_frame(AdapterSnapshot& snapshot,
                          const playback_graph::SubtitleFrameSnapshot& frame) noexcept;

}  // namespace kivo::video::player_runtime_adapter::runtime