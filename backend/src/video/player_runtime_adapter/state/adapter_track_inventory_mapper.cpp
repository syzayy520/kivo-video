#include "video/player_runtime_adapter/state/adapter_track_inventory_mapper.hpp"

#include <cstddef>

namespace kivo::video::player_runtime_adapter::runtime {

void apply_track_inventory(AdapterSnapshot& snapshot,
                           const playback_graph::TrackInventorySnapshot& inventory) noexcept {
    if (!inventory.valid) {
        return;
    }

    snapshot.selected_audio_track = inventory.selected_audio_track_id;
    snapshot.selected_video_track = inventory.selected_video_track_id;
    snapshot.selected_subtitle_track = inventory.selected_subtitle_track_id;

    snapshot.audio_track_ids.clear();
    snapshot.audio_track_ids.reserve(inventory.audio_track_count);
    for (std::size_t index = 0; index < inventory.audio_track_count; ++index) {
        snapshot.audio_track_ids.push_back(inventory.audio_track_ids[index]);
    }

    snapshot.video_track_ids.clear();
    snapshot.video_track_ids.reserve(inventory.video_track_count);
    for (std::size_t index = 0; index < inventory.video_track_count; ++index) {
        snapshot.video_track_ids.push_back(inventory.video_track_ids[index]);
    }

    snapshot.subtitle_track_ids.clear();
    snapshot.subtitle_track_ids.reserve(inventory.subtitle_track_count);
    for (std::size_t index = 0; index < inventory.subtitle_track_count; ++index) {
        snapshot.subtitle_track_ids.push_back(inventory.subtitle_track_ids[index]);
    }

    snapshot.connections.track_inventory = AdapterConnectionStatus::ConnectedToP7;
}

}  // namespace kivo::video::player_runtime_adapter::runtime
