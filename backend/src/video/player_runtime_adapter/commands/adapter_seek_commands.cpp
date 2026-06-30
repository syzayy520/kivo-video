#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

#include "video/player_runtime_adapter/commands/adapter_p7_request_mappers.hpp"
#include "video/player_runtime_adapter/p7_bridge/p7_command_result.hpp"

namespace kivo::video::player_runtime_adapter {

AdapterCommandResult PlayerRuntimeAdapter::seek(const std::int64_t position_ms) noexcept {
    return runtime::command_result_from_p7(
        session_.seek(commands::absolute_seek(position_ms)));
}

AdapterCommandResult PlayerRuntimeAdapter::seek_relative(const std::int64_t delta_ms) noexcept {
    return runtime::command_result_from_p7(
        session_.seek(commands::relative_seek(delta_ms)));
}

AdapterCommandResult PlayerRuntimeAdapter::step_forward(const std::int64_t step_ms) noexcept {
    return seek_relative(step_ms);
}

AdapterCommandResult PlayerRuntimeAdapter::step_backward(const std::int64_t step_ms) noexcept {
    return seek_relative(-step_ms);
}

}  // namespace kivo::video::player_runtime_adapter