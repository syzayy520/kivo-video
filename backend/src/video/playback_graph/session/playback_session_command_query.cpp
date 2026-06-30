#include "video/playback_graph/session/playback_session_runtime.hpp"

namespace kivo::video::playback_graph::runtime {

CommandLifecycleSnapshot PlaybackSessionRuntime::query_command(PlaybackCommandId id) const noexcept {
    return command_registry_.query(id);
}

SubscriptionToken PlaybackSessionRuntime::subscribe_events(GraphObserverHandle observer) noexcept {
    if (observer.callback == nullptr) {
        return SubscriptionToken{};
    }
    const auto token = next_subscription_token_;
    next_subscription_token_.value += 1;
    return token;
}

void PlaybackSessionRuntime::unsubscribe(SubscriptionToken token) noexcept {
    if (token.value != 0 && token.value < next_subscription_token_.value) {
        return;
    }
}

CommandToken PlaybackSessionRuntime::inject_system_event(const SystemEvent& event) noexcept {
    if (event.kind == SystemEventKind::None) {
        return reject(PlaybackGraphError::InvalidCommand);
    }
    return accept_command(GraphCommandKind::InjectSystemEvent, false);
}

CommandToken PlaybackSessionRuntime::accept_command(GraphCommandKind command,
                                                    bool requires_sidecar) noexcept {
    const auto conflict = resolve_command_conflict(command,
                                                   state_machine_.state(),
                                                   pending_transport_intent_.has_pending());
    if (conflict.decision == CommandConflictDecision::RejectClosedGraph ||
        conflict.decision == CommandConflictDecision::RejectInvalidState) {
        return reject(conflict.error);
    }

    CommandCreateRequest request{};
    request.kind = command;
    request.correlation_id = GraphCorrelationId{state_machine_.snapshot_revision().value + 1};
    request.accepted_graph_revision = state_machine_.snapshot_revision();
    request.requires_sidecar = requires_sidecar;

    const auto created = command_registry_.create(request, p4_adapter_);
    if (!created.accepted()) {
        return reject(created.error);
    }

    if (conflict.creates_pending_transport_intent) {
        pending_transport_intent_.request(command);
    }
    return created.token;
}

CommandToken PlaybackSessionRuntime::reject(PlaybackGraphError error) const noexcept {
    CommandToken token{};
    token.immediate_error = error;
    return token;
}

void PlaybackSessionRuntime::complete_if_accepted(CommandToken token,
                                                  CommandTerminalStatus terminal_status,
                                                  PlaybackGraphError error) noexcept {
    if (token.accepted()) {
        (void)command_registry_.complete(token.command_id, terminal_status, error);
    }
}

}  // namespace kivo::video::playback_graph::runtime