#include "video/subtitle_plane/observer/observer_registry.hpp"

namespace kivo::video::subtitle_plane::runtime {

KivoP8ApiStatus subscribe_observer(
    PlaneStateRecord& record,
    const KivoSubtitleObserverCallbacks& callbacks,
    KivoSubtitleSubscriptionToken* const out_token) noexcept {
    if (callbacks.on_event == nullptr) {
        out_token->value = 0;
        return KivoP8ApiStatus_InvalidArgument;
    }
    if (record.observer_subscription_token != 0) {
        record.metrics.observer_queue_overflow_count += 1;
        out_token->value = 0;
        return KivoP8ApiStatus_Failed;
    }
    record.observer_callbacks = callbacks;
    record.observer_subscription_token = 1;
    out_token->value = 1;
    return KivoP8ApiStatus_Ok;
}

KivoP8ApiStatus unsubscribe_observer(
    PlaneStateRecord& record,
    const KivoSubtitleSubscriptionToken token) noexcept {
    if (token.value == 0 || token.value != record.observer_subscription_token) {
        return KivoP8ApiStatus_InvalidArgument;
    }
    record.observer_subscription_token = 0;
    record.observer_callbacks = KivoSubtitleObserverCallbacks{};
    return KivoP8ApiStatus_Ok;
}

bool observer_mutation_blocked(const PlaneStateRecord& record) noexcept {
    return record.observer_dispatching != 0u;
}

void dispatch_observer_event(PlaneStateRecord& record,
                             const KivoSubtitleEvidenceReason reason,
                             const KivoSubtitleEvidenceSeverity severity) noexcept {
    if (record.observer_subscription_token == 0 ||
        record.observer_callbacks.on_event == nullptr) {
        return;
    }
    KivoSubtitleEvent event{};
    event.revision = record.snapshot_revision;
    event.reason = reason;
    event.severity = severity;
    record.observer_dispatching = 1;
    record.observer_callbacks.on_event(record.observer_callbacks.user_data, &event);
    record.observer_dispatching = 0;
}

}  // namespace kivo::video::subtitle_plane::runtime
