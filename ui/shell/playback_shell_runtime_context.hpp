#pragma once

#include <memory>

#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"
#include "playback_shell_runtime_adapter_bridge.hpp"

namespace kivo::ui::shell {

class PlaybackShellRuntimeContext final {
public:
    PlaybackShellRuntimeContext();
    ~PlaybackShellRuntimeContext();

    [[nodiscard]] PlaybackShellRuntimeAdapterBridge* bridge() noexcept;

private:
    std::unique_ptr<kivo::video::player_runtime_adapter::PlayerRuntimeAdapter> adapter_;
    std::unique_ptr<PlaybackShellRuntimeAdapterBridge> bridge_;
};

}  // namespace kivo::ui::shell