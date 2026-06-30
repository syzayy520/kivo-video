#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter::runtime {

AdapterCommandResult command_result_from_p7(playback_graph::CommandToken token) noexcept {
    AdapterCommandResult result{};
    result.p7_error = token.immediate_error;
    result.p7_command_id = token.command_id;
    result.status =
        token.accepted() ? AdapterCommandStatus::Accepted : AdapterCommandStatus::RejectedByP7;
    return result;
}

AdapterCommandResult missing_p7_api(AdapterMissingP7Api missing_api) noexcept {
    AdapterCommandResult result{};
    result.status = AdapterCommandStatus::NotConnectedToP7;
    result.missing_api = missing_api;
    return result;
}

}  // namespace kivo::video::player_runtime_adapter::runtime
