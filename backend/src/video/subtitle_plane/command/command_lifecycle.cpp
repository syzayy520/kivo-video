#include "video/subtitle_plane/command/command_lifecycle.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoSubtitleCommandToken reject_command(const KivoSubtitleCorrelationId correlation_id,
                                        const KivoSubtitlePlaneError error) noexcept {
    KivoSubtitleCommandToken token{};
    token.correlation_id = correlation_id;
    token.immediate_status = KivoSubtitleCommandImmediateStatus_Rejected;
    token.immediate_error = error;
    return token;
}

KivoSubtitleCommandToken accept_command(PlaneStateRecord& plane,
                                        const KivoSubtitleCorrelationId correlation_id,
                                        const KivoSubtitleCommandImmediateStatus status,
                                        const KivoSubtitlePlaneError error) noexcept {
    KivoSubtitleCommandToken token{};
    token.command_id.value = plane.next_command_id++;
    token.correlation_id = correlation_id;
    token.immediate_status = status;
    token.immediate_error = error;
    token.accepted_snapshot_revision.value = ++plane.snapshot_revision.value;

    plane.last_command = KivoSubtitleCommandLifecycleSnapshot{};
    plane.last_command.command_id = token.command_id;
    plane.last_command.correlation_id = correlation_id;
    plane.last_command.state = error == KivoSubtitlePlaneError_None
        ? KivoSubtitleCommandLifecycleState_Completed
        : KivoSubtitleCommandLifecycleState_Failed;
    plane.last_command.error = error;
    plane.last_command.accepted_snapshot_revision = token.accepted_snapshot_revision;
    plane.last_command.last_update_revision = token.accepted_snapshot_revision;
    plane.last_command.accepted_tick = token.command_id.value;
    plane.last_command.completed_tick = token.command_id.value + 1;
    return token;
}

KivoSubtitleCommandLifecycleSnapshot query_command_snapshot(
    const PlaneStateRecord& plane,
    const KivoSubtitleCommandId command_id) noexcept {
    if (command_id.value == 0) {
        KivoSubtitleCommandLifecycleSnapshot snapshot{};
        snapshot.state = KivoSubtitleCommandLifecycleState_Failed;
        snapshot.error = KivoSubtitlePlaneError_InvalidArgument;
        return snapshot;
    }
    if (plane.last_command.command_id.value != command_id.value) {
        KivoSubtitleCommandLifecycleSnapshot snapshot{};
        snapshot.command_id = command_id;
        snapshot.state = KivoSubtitleCommandLifecycleState_Unknown;
        return snapshot;
    }
    return plane.last_command;
}

}  // namespace kivo::video::subtitle_plane::runtime
