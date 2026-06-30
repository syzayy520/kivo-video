#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/player_runtime_adapter/state/adapter_state_mapper.hpp"

namespace kivo::video::player_runtime_adapter {

PlayerRuntimeAdapter::PlayerRuntimeAdapter() = default;

PlayerRuntimeAdapter::PlayerRuntimeAdapter(const playback_graph::PlaybackGraphPolicy& policy)
    : session_(policy) {}

PlayerRuntimeAdapter::~PlayerRuntimeAdapter() = default;

PlayerRuntimeAdapter::PlayerRuntimeAdapter(PlayerRuntimeAdapter&&) noexcept = default;

PlayerRuntimeAdapter& PlayerRuntimeAdapter::operator=(PlayerRuntimeAdapter&&) noexcept = default;

AdapterSnapshot PlayerRuntimeAdapter::snapshot() const noexcept {
    return runtime::map_snapshot(session_, rebuffer_count_);
}

}  // namespace kivo::video::player_runtime_adapter