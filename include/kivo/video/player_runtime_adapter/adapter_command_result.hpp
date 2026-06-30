#pragma once

#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/player_runtime_adapter/adapter_connection_status.hpp"

namespace kivo::video::player_runtime_adapter {

struct AdapterCommandResult {
    AdapterCommandStatus status{AdapterCommandStatus::RejectedByP7};
    AdapterMissingP7Api missing_api{AdapterMissingP7Api::None};
    playback_graph::PlaybackGraphError p7_error{playback_graph::PlaybackGraphError::None};
    playback_graph::PlaybackCommandId p7_command_id{};

    [[nodiscard]] bool accepted() const noexcept {
        return status == AdapterCommandStatus::Accepted;
    }
};

}  // namespace kivo::video::player_runtime_adapter
