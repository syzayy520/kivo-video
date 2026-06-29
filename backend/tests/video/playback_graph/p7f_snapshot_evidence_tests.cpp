#include <cstring>

#include "video/playback_graph/evidence/critical_proof_ring.hpp"
#include "video/playback_graph/evidence/layered_error_taxonomy.hpp"
#include "video/playback_graph/evidence/privacy_salt_lifecycle.hpp"
#include "video/playback_graph/health/graph_health_monitor.hpp"
#include "video/playback_graph/metrics/graph_metrics_budget.hpp"
#include "video/playback_graph/observer/observer_registry.hpp"
#include "video/playback_graph/snapshot/snapshot_store.hpp"

using namespace kivo::video::playback_graph;
using namespace kivo::video::playback_graph::runtime;

namespace {

void observer_callback(void* user_data, const PlaybackGraphEvent&) noexcept {
    auto* count = static_cast<int*>(user_data);
    *count += 1;
}

[[nodiscard]] bool verify_snapshot_store() {
    SnapshotStore store{};
    PlaybackSessionSnapshot snapshot{};
    snapshot.state = PlaybackGraphState::Playing;
    const auto revision = store.publish(snapshot);
    auto latest = store.latest_session_snapshot();
    latest.state = PlaybackGraphState::Failed;
    if (store.latest_session_snapshot().state != PlaybackGraphState::Playing) {
        return false;
    }
    return revision.value == 1 && store.query_audio_queue().ok() &&
           store.query_video_queue().ok() && store.query_clock().ok() &&
           store.retained_snapshot_count() == 1;
}

[[nodiscard]] bool verify_observer_registry() {
    ObserverRegistry observers{};
    int delivered = 0;
    GraphObserverHandle handle{};
    handle.user_data = &delivered;
    handle.callback = observer_callback;
    const auto token = observers.subscribe(handle, 1);
    if (token.value == 0) {
        return false;
    }
    PlaybackGraphEvent event{};
    event.kind = PlaybackGraphEventKind::GraphStateChanged;
    event.state = PlaybackGraphState::Closed;
    if (!observers.publish(event)) {
        return false;
    }
    if (observers.publish(event)) {
        return false;
    }
    if (observers.deliver(ObserverDeliveryThread::GraphOwnerThread)) {
        return false;
    }
    if (!observers.deliver(ObserverDeliveryThread::SafeDeliveryThread) || delivered != 1) {
        return false;
    }
    observers.unsubscribe(token);
    observers.detach_all_on_close();
    return observers.stats().overflow_count == 1 && observers.stats().closed_snapshot_delivered;
}

[[nodiscard]] bool verify_critical_proof_ring() {
    CriticalProofRing ring{1, 2};
    ring.set_persistent_sink_available(false);
    if (!ring.push(
            CriticalProof{CriticalProofKind::Close, PlaybackCommandId{1}, PlaybackGraphError::None})) {
        return false;
    }
    if (!ring.push(CriticalProof{CriticalProofKind::Recovery,
                                 PlaybackCommandId{2},
                                 PlaybackGraphError::RecoveryFailed})) {
        return false;
    }
    if (!ring.push(CriticalProof{CriticalProofKind::Fatal,
                                 PlaybackCommandId{3},
                                 PlaybackGraphError::RecoveryFailed})) {
        return false;
    }
    return ring.sink_failure_count() == 3 && ring.contains(CriticalProofKind::Close) &&
           ring.contains(CriticalProofKind::Recovery) && ring.contains(CriticalProofKind::Fatal);
}

[[nodiscard]] bool verify_metrics_health_budget() {
    GraphMetricsBudget budget{};
    PlaybackGraphPolicy policy{};
    if (!budget.validate(policy, true).accepted || budget.required_policy_field_count() != 30) {
        return false;
    }
    policy.tuning.seqlock_max_attempt_count = 0;
    if (budget.validate(policy, false).error != PlaybackGraphError::PolicyAdjustedNonCritical) {
        return false;
    }
    if (budget.validate(policy, true).error != PlaybackGraphError::PolicyRejectedSafetyCritical) {
        return false;
    }
    GraphHealthMonitor health{};
    health.sample(GraphHealthSample{false, false, 0, 0});
    if (!health.unhealthy()) {
        return false;
    }
    health.sample(GraphHealthSample{true, false, 100, 3});
    return !health.unhealthy();
}

[[nodiscard]] bool verify_privacy_salt() {
    PrivacySaltLifecycle salt{};
    salt.generate_for_device(42);
    const auto first_id = salt.salt_id();
    const auto redacted = salt.redact_source_id(100);
    if (std::strstr(redacted.text.data(), "100") != nullptr) {
        return false;
    }
    salt.rotate(43);
    if (salt.salt_id() == first_id) {
        return false;
    }
    const auto crash = salt.crash_report_tag();
    return std::strstr(crash.text.data(), "salt_id:") != nullptr &&
           !salt.crash_report_contains_raw_salt();
}

[[nodiscard]] bool verify_error_taxonomy() {
    if (!layered_error_models_are_distinct()) {
        return false;
    }
    if (severity_for_command_error(PlaybackGraphError::InvalidState) !=
        EvidenceSeverity::CommandRejected) {
        return false;
    }
    if (evidence_reason_for_late_discard() != GraphEvidenceReason::LateCallbackDiscarded) {
        return false;
    }
    return !architecture_failure_user_facing(ArchitectureGuardFailure::PublicHeaderPuristFailed);
}

}  // namespace

int main() {
    if (!verify_snapshot_store()) {
        return 1;
    }
    if (!verify_observer_registry()) {
        return 2;
    }
    if (!verify_critical_proof_ring()) {
        return 3;
    }
    if (!verify_metrics_health_budget()) {
        return 4;
    }
    if (!verify_privacy_salt()) {
        return 5;
    }
    if (!verify_error_taxonomy()) {
        return 6;
    }
    return 0;
}
