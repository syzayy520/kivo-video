#include "video/player_runtime_adapter/state/adapter_track_inventory_mapper.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

void apply_track_inventory(AdapterSnapshot& snapshot,
                           const playback_graph::TrackInventorySnapshot& inventory) noexcept {
    if (!inventory.valid) {
        return;
    }

    snapshot.selected_audio_track = inventory.selected_audio_track_id;
    snapshot.connections.track_inventory = AdapterConnectionStatus::ConnectedToP7;
}

}  // namespace kivo::video::player_runtime_adapter::runtime