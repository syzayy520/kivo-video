#pragma once

#include "kivo/video/playback_graph/track/track_inventory_snapshot.hpp"
#include "kivo/video/player_runtime_adapter/adapter_snapshot.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

void apply_track_inventory(AdapterSnapshot& snapshot,
                         const playback_graph::TrackInventorySnapshot& inventory) noexcept;

}  // namespace kivo::video::player_runtime_adapter::runtime