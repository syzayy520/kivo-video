#include "kivo/video/player_runtime_adapter/player_runtime_adapter.hpp"

namespace kivo::video::player_runtime_adapter {

AdapterCommandResult PlayerRuntimeAdapter::copy_diagnostics() noexcept {
    const auto summary = session_.query_diagnostics_summary();
    if (!summary.valid) {
        AdapterCommandResult result{};
        result.status = AdapterCommandStatus::RejectedByP7;
        if (summary.invalid_reason == playback_graph::DiagnosticsInvalidReason::GraphClosed) {
            result.p7_error = playback_graph::PlaybackGraphError::ClosedGraph;
        } else {
            result.p7_error = playback_graph::PlaybackGraphError::InvalidState;
        }
        return result;
    }
    AdapterCommandResult result{};
    result.status = AdapterCommandStatus::Accepted;
    return result;
}

}  // namespace kivo::video::player_runtime_adapter