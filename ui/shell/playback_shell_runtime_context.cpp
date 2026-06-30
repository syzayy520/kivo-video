#include "playback_shell_runtime_context.hpp"

namespace kivo::ui::shell {

PlaybackShellRuntimeContext::PlaybackShellRuntimeContext() {
    adapter_ = std::make_unique<kivo::video::player_runtime_adapter::PlayerRuntimeAdapter>();
    bridge_ = std::make_unique<PlaybackShellRuntimeAdapterBridge>(adapter_.get());
}

PlaybackShellRuntimeContext::~PlaybackShellRuntimeContext() = default;

PlaybackShellRuntimeAdapterBridge* PlaybackShellRuntimeContext::bridge() noexcept {
    return bridge_.get();
}

kivo::video::player_runtime_adapter::PlayerRuntimeAdapter*
PlaybackShellRuntimeContext::adapter() noexcept {
    return adapter_.get();
}

}  // namespace kivo::ui::shell