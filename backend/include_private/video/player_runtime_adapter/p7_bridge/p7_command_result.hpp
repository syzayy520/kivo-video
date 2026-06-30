#pragma once

#include "kivo/video/playback_graph/command_token.hpp"
#include "kivo/video/player_runtime_adapter/adapter_command_result.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

[[nodiscard]] AdapterCommandResult command_result_from_p7(
    playback_graph::CommandToken token) noexcept;

[[nodiscard]] AdapterCommandResult missing_p7_api(AdapterMissingP7Api missing_api) noexcept;

}  // namespace kivo::video::player_runtime_adapter::runtime
