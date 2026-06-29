#include <cstdint>

#include "video/playback_graph/attachment/attachment_operation_registry.hpp"
#include "video/playback_graph/attachment/phase_bitset.hpp"
#include "video/playback_graph/events/critical_event_mpsc_ring.hpp"
#include "video/playback_graph/generation/generation_stamp_publisher.hpp"
#include "video/playback_graph/internal/hot_path_contract_guard.hpp"
#include "video/playback_graph/realtime/realtime_event_allocator.hpp"

using namespace kivo::video::playback_graph;
using namespace kivo::video::playback_graph::runtime;

namespace {

[[nodiscard]] GenerationStamp active_generation(std::uint64_t tuple) noexcept {
    GenerationStamp stamp{};
    stamp.tuple_id.value = tuple;
    stamp.stream_stamp_id = tuple;
    stamp.state = GenerationStampState::Active;
    return stamp;
}

[[nodiscard]] OperationKey operation_key(std::uint64_t session,
                                         std::uint64_t graph,
                                         std::uint64_t attachment,
                                         AttachmentKind attachment_kind,
                                         std::uint64_t operation,
                                         AttachmentOperationKind operation_kind) noexcept {
    OperationKey key{};
    key.session_id.value = session;
    key.graph_id.value = graph;
    key.attachment_instance_id.value = attachment;
    key.attachment_kind = attachment_kind;
    key.operation_id.value = operation;
    key.operation_kind = operation_kind;
    return key;
}

[[nodiscard]] AttachmentAsyncToken token_for(const OperationKey& key,
                                             const GenerationStamp& stamp) noexcept {
    AttachmentAsyncToken token{};
    token.key = key;
    token.generation = stamp;
    token.command_id.value = key.operation_id.value;
    token.correlation_id.value = key.operation_id.value + 100;
    return token;
}

[[nodiscard]] bool verify_operation_key_identity() {
    const auto key = operation_key(1, 2, 3, AttachmentKind::Audio, 4, AttachmentOperationKind::AudioSeek);
    const auto same_operation_old_session =
        operation_key(99, 2, 3, AttachmentKind::Audio, 4, AttachmentOperationKind::AudioSeek);
    AttachmentOperationRegistry registry{2, 2, 1};
    if (!registry.start_operation(token_for(key, active_generation(1)), false)) {
        return false;
    }
    if (!registry.contains_active(key)) {
        return false;
    }
    if (registry.contains_active(same_operation_old_session)) {
        return false;
    }
    if (!registry.mark_timeout(key,
                               AttachmentTimeoutReason::OperationTimeout,
                               PlaybackGraphError::AttachmentCompletionTimeout)) {
        return false;
    }
    return !registry.start_operation(token_for(key, active_generation(1)), false);
}

[[nodiscard]] bool verify_completion_event_flow() {
    const auto key =
        operation_key(1, 1, 7, AttachmentKind::Audio, 9, AttachmentOperationKind::AudioSeek);
    const auto stamp = active_generation(12);
    AttachmentOperationRegistry registry{2, 1, 1};
    const auto token = token_for(key, stamp);
    if (token.generation.state != GenerationStampState::Active || token.command_id.value == 0 ||
        token.correlation_id.value == 0) {
        return false;
    }
    if (!registry.start_operation(token, false)) {
        return false;
    }
    AttachmentCompletionEvent event{};
    event.token = token;
    event.status = AttachmentCompletionStatus::Succeeded;
    if (!registry.enqueue_completion(event)) {
        return false;
    }
    const auto popped = registry.pop_completion();
    return popped.has_value() && popped->token.key == key &&
           popped->status == AttachmentCompletionStatus::Succeeded;
}

[[nodiscard]] bool verify_completion_timeout_tombstone() {
    AttachmentOperationRegistry registry{2, 1, 1};
    const auto key = operation_key(1, 1, 1, AttachmentKind::Video, 1, AttachmentOperationKind::VideoSeek);
    const auto token = token_for(key, active_generation(1));
    if (!registry.start_operation(token, false)) {
        return false;
    }
    AttachmentCompletionEvent event{};
    event.token = token;
    if (!registry.enqueue_completion(event)) {
        return false;
    }
    if (registry.enqueue_completion(event)) {
        return false;
    }
    if (registry.queue_full_drop_count() != 1) {
        return false;
    }
    if (!registry.mark_timeout(key, AttachmentTimeoutReason::PhaseTimeout, PlaybackGraphError::SeekTimeout)) {
        return false;
    }
    if (registry.mark_timeout(key, AttachmentTimeoutReason::CommandTimeout, PlaybackGraphError::SeekTimeout)) {
        return false;
    }
    if (registry.enqueue_completion(event)) {
        return false;
    }
    return registry.late_completion_discard_count() == 1 && registry.tombstone_count() == 1;
}

[[nodiscard]] bool verify_phase_bitset() {
    PhaseBitset phase{};
    const auto stamp = active_generation(1);
    for (std::uint64_t index = 0; index < PhaseBitset::kMaxOperationCount; ++index) {
        if (!phase.add_required(operation_key(1,
                                             1,
                                             index + 1,
                                             AttachmentKind::Video,
                                             index + 1,
                                             AttachmentOperationKind::VideoSeek),
                                stamp,
                                AttachmentOperationKind::VideoSeek)) {
            return false;
        }
    }
    if (phase.add_required(operation_key(1, 1, 99, AttachmentKind::Video, 99, AttachmentOperationKind::VideoSeek),
                           stamp,
                           AttachmentOperationKind::VideoSeek)) {
        return false;
    }

    const auto key =
        operation_key(1, 1, 1, AttachmentKind::Video, 1, AttachmentOperationKind::VideoSeek);
    if (phase.complete(key, stamp, AttachmentOperationKind::AudioSeek,
                       AttachmentCompletionStatus::Succeeded) !=
        PhaseCompletionResult::WrongOperationKind) {
        return false;
    }
    if (phase.complete(key, active_generation(2), AttachmentOperationKind::VideoSeek,
                       AttachmentCompletionStatus::Succeeded) !=
        PhaseCompletionResult::WrongGeneration) {
        return false;
    }
    if (phase.complete(key, stamp, AttachmentOperationKind::VideoSeek,
                       AttachmentCompletionStatus::Succeeded) != PhaseCompletionResult::Accepted) {
        return false;
    }
    return phase.complete(key, stamp, AttachmentOperationKind::VideoSeek,
                          AttachmentCompletionStatus::Succeeded) ==
           PhaseCompletionResult::DuplicateCompletion;
}

[[nodiscard]] bool verify_generation_stamp_publisher() {
    GenerationStampPublisher publisher{};
    publisher.publish_pending_barrier(GenerationTupleId{1}, 10);
    const auto pending = publisher.read(3);
    if (!pending.stable || GenerationStampPublisher::hot_path_accepts(pending.stamp)) {
        return false;
    }
    publisher.publish_active(GenerationTupleId{2}, 11);
    const auto active = publisher.read(3);
    if (!active.stable || !GenerationStampPublisher::hot_path_accepts(active.stamp) ||
        active.stamp.tuple_id.value != 2 || active.stamp.stream_stamp_id != 11) {
        return false;
    }
    publisher.publish_retired(GenerationTupleId{3}, 12);
    if (GenerationStampPublisher::hot_path_accepts(publisher.read(3).stamp)) {
        return false;
    }
    publisher.begin_unstable_write_for_test();
    const auto unstable = publisher.read(3);
    publisher.finish_unstable_write_for_test(GenerationTupleId{4}, 13, GenerationStampState::Active);
    return !unstable.stable && unstable.attempts == 3;
}

[[nodiscard]] bool verify_critical_event_ring() {
    CriticalEventMpscRing ring{1, 1};
    CriticalEvent first{};
    first.error = PlaybackGraphError::None;
    if (!ring.push(first)) {
        return false;
    }
    CriticalEvent nonfatal{};
    nonfatal.error = PlaybackGraphError::SeekTimeout;
    if (ring.push(nonfatal)) {
        return false;
    }
    CriticalEvent fatal{};
    fatal.severity = CriticalEventSeverity::Fatal;
    fatal.error = PlaybackGraphError::RecoveryFailed;
    if (!ring.push(fatal) || !ring.overflow_latched() || ring.nonfatal_drop_count() != 1 ||
        ring.fatal_reserved_count() != 1) {
        return false;
    }
    return ring.pop_by_graph_owner().has_value() && ring.pop_by_graph_owner().has_value();
}

[[nodiscard]] bool verify_realtime_allocator() {
    RealtimeEventAllocator allocator{2};
    auto* first = allocator.allocate();
    auto* second = allocator.allocate();
    if (first == nullptr || second == nullptr || allocator.allocate() != nullptr ||
        allocator.overflow_count() != 1) {
        return false;
    }
    allocator.release(first);
    if (allocator.allocate() == nullptr) {
        return false;
    }
    allocator.mark_graph_started();
    return !allocator.debug_dynamic_allocation_allowed() && allocator.capacity() == 2;
}

[[nodiscard]] bool verify_hot_path_guard() {
    HotPathContractGuard guard{};
    if (!guard.record_logging_attempt()) {
        return false;
    }
    guard.enter_hot_path();
    if (guard.record_allocation_attempt() || guard.last_violation() != HotPathViolation::Allocation) {
        return false;
    }
    if (guard.record_blocking_attempt() || guard.last_violation() != HotPathViolation::Blocking) {
        return false;
    }
    if (guard.record_logging_attempt() || guard.last_violation() != HotPathViolation::Logging) {
        return false;
    }
    if (guard.record_mutable_graph_access_attempt() ||
        guard.last_violation() != HotPathViolation::MutableGraphAccess) {
        return false;
    }
    guard.leave_hot_path();
    return guard.record_unbounded_retry_attempt();
}

}  // namespace

int main() {
    if (!verify_operation_key_identity()) {
        return 1;
    }
    if (!verify_completion_event_flow()) {
        return 2;
    }
    if (!verify_completion_timeout_tombstone()) {
        return 3;
    }
    if (!verify_phase_bitset()) {
        return 4;
    }
    if (!verify_generation_stamp_publisher()) {
        return 5;
    }
    if (!verify_critical_event_ring()) {
        return 6;
    }
    if (!verify_realtime_allocator()) {
        return 7;
    }
    if (!verify_hot_path_guard()) {
        return 8;
    }
    return 0;
}
