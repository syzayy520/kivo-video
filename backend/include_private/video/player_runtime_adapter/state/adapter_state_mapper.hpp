#pragma once

#include <cstdint>

#include "kivo/video/playback_graph/playback_session.hpp"
#include "kivo/video/player_runtime_adapter/adapter_snapshot.hpp"
#include "kivo/video/player_runtime_adapter/adapter_types.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

[[nodiscard]] RuntimePlaybackState map_playback_state(
    playback_graph::PlaybackGraphState state) noexcept;

[[nodiscard]] BufferReason map_buffer_reason(playback_graph::PlaybackGraphState state) noexcept;

[[nodiscard]] AdapterSnapshot map_snapshot(const playback_graph::PlaybackSession& session,
                                           std::uint32_t rebuffer_count) noexcept;

}  // namespace kivo::video::player_runtime_adapter::runtime
