#include "kivo/playback/control_plane/recovery/playback_failure_kind.hpp"
#include "kivo/playback/control_plane/recovery/playback_failure.hpp"
#include "kivo/playback/control_plane/recovery/failure_taxonomy_policy.hpp"
#include "kivo/playback/control_plane/recovery/recovery_context.hpp"
#include "kivo/playback/control_plane/recovery/recovery_budget.hpp"
#include "kivo/playback/control_plane/recovery/authorized_fallback.hpp"
#include "kivo/playback/control_plane/recovery/drm_failure_policy.hpp"
#include "kivo/playback/control_plane/quarantine/quarantined_reader_record.hpp"
#include "kivo/playback/control_plane/lifetime/shutdown_guard.hpp"
#include "kivo/playback/control_plane/events/event_delivery_class.hpp"
#include "kivo/playback/control_plane/events/event_bus_policy.hpp"
#include "kivo/playback/control_plane/events/session_event.hpp"
#include "kivo/playback/control_plane/control/critical_event_queue.hpp"
#include "kivo/playback/control_plane/diagnostics/stable_reason_code.hpp"
#include "kivo/playback/control_plane/diagnostics/safe_diagnostic_string.hpp"
#include "kivo/playback/control_plane/diagnostics/fixed_window_histogram.hpp"
#include "kivo/playback/control_plane/diagnostics/session_diagnostics.hpp"
#include "kivo/playback/control_plane/reader/low_speed_policy.hpp"
#include "kivo/playback/control_plane/reader/reader_stall_policy.hpp"
#include "kivo/playback/control_plane/control/flush_scope.hpp"
#include "kivo/playback/control_plane/runtime/resource_budget.hpp"

#include <cassert>
#include <iostream>

using namespace kivo::playback::control_plane;

// P4G-051: Recovery
static void test_recovery_decision_kind()
{
    recovery::RecoveryDecisionKind k = recovery::RecoveryDecisionKind::RetryRead;
    assert(k == recovery::RecoveryDecisionKind::RetryRead);
    std::cout << "PASS: recovery_decision_kind\n";
}

static void test_recovery_context()
{
    recovery::RecoveryContext ctx;
    ctx.recovery_epoch = revision::RecoveryEpoch{1};
    ctx.root_failure.kind = recovery::PlaybackFailureKind::ReaderReadFailed;
    assert(ctx.recovery_epoch.value == 1);
    std::cout << "PASS: recovery_context\n";
}

static void test_recovery_budget()
{
    recovery::RecoveryBudget b;
    b.max_retry_count = primitives::Count{3};
    b.max_fallback_count = primitives::Count{1};
    b.allow_destructive_cancel = false;
    assert(b.max_retry_count.value == 3);
    assert(!b.allow_destructive_cancel);
    std::cout << "PASS: recovery_budget\n";
}

static void test_authorized_fallback()
{
    recovery::AuthorizedFallback f;
    f.fallback_id = 7;
    f.authorization_kind = track::FallbackAuthorizationKind::DifferentCandidate;
    assert(f.fallback_id == 7);
    std::cout << "PASS: authorized_fallback\n";
}

// P4G-052: Failure Taxonomy
static void test_playback_failure_kind()
{
    recovery::PlaybackFailureKind k = recovery::PlaybackFailureKind::ReaderTimeout;
    assert(k == recovery::PlaybackFailureKind::ReaderTimeout);
    std::cout << "PASS: playback_failure_kind\n";
}

static void test_playback_failure()
{
    recovery::PlaybackFailure f;
    f.kind = recovery::PlaybackFailureKind::NetworkError;
    f.severity = recovery::FailureSeverity::Recoverable;
    f.origin = recovery::FailureOrigin::Reader;
    f.recoverable = true;
    assert(f.kind == recovery::PlaybackFailureKind::NetworkError);
    assert(f.recoverable);
    std::cout << "PASS: playback_failure\n";
}

static void test_failure_taxonomy_policy()
{
    recovery::FailureTaxonomyPolicy p;
    p.expose_network_errors_to_user = false;
    p.classify_drm_failure_as_user_visible = true;
    assert(!p.expose_network_errors_to_user);
    std::cout << "PASS: failure_taxonomy_policy\n";
}

// P4G-053: DRM Failure Policy
static void test_drm_failure_policy()
{
    recovery::DrmFailurePolicy p;
    assert(p.default_recovery_decision == recovery::RecoveryDecisionKind::HardTerminateToUpperLayer);
    assert(p.stop_scheduling_new_read);
    std::cout << "PASS: drm_failure_policy\n";
}

// P4G-054: Quarantine / ShutdownGuard
static void test_quarantined_reader_record()
{
    quarantine::QuarantinedReaderRecord r;
    r.reader_id = revision::ReaderInstanceId{1};
    r.callbacks_permanently_ignored = true;
    assert(r.reader_id.value == 1);
    assert(r.callbacks_permanently_ignored);
    std::cout << "PASS: quarantined_reader_record\n";
}

static void test_shutdown_quiescence_policy()
{
    lifetime::ShutdownQuiescencePolicy p;
    p.total_timeout_ms = primitives::Milliseconds{5000};
    p.allow_quarantine = true;
    assert(p.total_timeout_ms.value == 5000);
    std::cout << "PASS: shutdown_quiescence_policy\n";
}

// P4G-055: EventBus
static void test_event_delivery_class()
{
    events::EventDeliveryClass c = events::EventDeliveryClass::Critical;
    assert(c == events::EventDeliveryClass::Critical);
    std::cout << "PASS: event_delivery_class\n";
}

static void test_event_bus_policy()
{
    events::EventBusPolicy p;
    p.max_pending_events_per_subscriber = primitives::Count{256};
    p.drop_noncritical_when_full = true;
    assert(p.max_pending_events_per_subscriber.value == 256);
    std::cout << "PASS: event_bus_policy\n";
}

static void test_session_event()
{
    events::SessionEvent e;
    e.type = events::SessionEventType::ErrorRaised;
    e.delivery_class = events::EventDeliveryClass::Critical;
    assert(e.type == events::SessionEventType::ErrorRaised);
    assert(e.delivery_class == events::EventDeliveryClass::Critical);
    std::cout << "PASS: session_event\n";
}

// P4G-056: CriticalEventQueue
static void test_control_event_queue_policy()
{
    control::ControlEventQueuePolicy p;
    p.normal_max_depth = primitives::Count{128};
    p.critical_reserved_depth = primitives::Count{16};
    assert(p.normal_max_depth.value == 128);
    std::cout << "PASS: control_event_queue_policy\n";
}

static void test_critical_event_queue_kind()
{
    control::CriticalEventQueueKind k = control::CriticalEventQueueKind::Critical;
    assert(k == control::CriticalEventQueueKind::Critical);
    std::cout << "PASS: critical_event_queue_kind\n";
}

// P4G-057: Diagnostics
static void test_stable_reason_code()
{
    diagnostics::StableReasonCode c;
    c.value = 42;
    assert(c.value == 42);
    std::cout << "PASS: stable_reason_code\n";
}

static void test_safe_diagnostic_string()
{
    diagnostics::SafeDiagnosticString s;
    s.redacted = true;
    s.sensitivity = diagnostics::DiagnosticSensitivity::RedactedUrl;
    assert(s.redacted);
    std::cout << "PASS: safe_diagnostic_string\n";
}

static void test_fixed_window_histogram()
{
    diagnostics::FixedWindowHistogram<16, 4> h;
    h.update(0, 100, primitives::MonotonicTimestamp{0});
    h.update(0, 200, primitives::MonotonicTimestamp{0});
    assert(h.total_for_bucket(0) == 300);
    assert(h.overflow_count() == 0);
    std::cout << "PASS: fixed_window_histogram\n";
}

static void test_session_diagnostics()
{
    diagnostics::SessionDiagnostics d;
    d.session_id = revision::SessionId{1};
    d.seek_count = 5;
    assert(d.session_id.value == 1);
    assert(d.seek_count == 5);
    std::cout << "PASS: session_diagnostics\n";
}

// P4G-058: ReaderStall / LowSpeed
static void test_low_speed_policy()
{
    reader::LowSpeedPolicy p;
    p.enable_low_speed_detection = true;
    p.min_bytes_per_second = primitives::BytesPerSecond{50000};
    assert(p.enable_low_speed_detection);
    std::cout << "PASS: low_speed_policy\n";
}

static void test_reader_stall_policy()
{
    reader::ReaderStallPolicy p;
    p.max_no_byte_age_ms = primitives::Milliseconds{10000};
    p.escalate_to_recovery = true;
    assert(p.max_no_byte_age_ms.value == 10000);
    std::cout << "PASS: reader_stall_policy\n";
}

// P4G-059: FlushScope table
static void test_flush_scope_buffer_only()
{
    auto s = control::flush_scope_semantics(control::FlushScopeKind::BufferOnly);
    assert(!s.cancel_read);
    assert(s.clear_buffer);
    assert(!s.increment_stream_revision);
    std::cout << "PASS: flush_scope_buffer_only\n";
}

static void test_flush_scope_reader_in_flight()
{
    auto s = control::flush_scope_semantics(control::FlushScopeKind::ReaderInFlight);
    assert(s.cancel_read);
    assert(s.increment_read_epoch);
    assert(!s.increment_stream_revision);
    std::cout << "PASS: flush_scope_reader_in_flight\n";
}

static void test_flush_scope_stream_revision()
{
    auto s = control::flush_scope_semantics(control::FlushScopeKind::StreamRevision);
    assert(s.cancel_read);
    assert(s.clear_buffer);
    assert(s.increment_read_epoch);
    assert(s.increment_stream_revision);
    std::cout << "PASS: flush_scope_stream_revision\n";
}

static void test_flush_scope_full_session()
{
    auto s = control::flush_scope_semantics(control::FlushScopeKind::FullSession);
    assert(s.cancel_read);
    assert(s.clear_buffer);
    assert(s.increment_stream_revision);
    std::cout << "PASS: flush_scope_full_session\n";
}

// P4G-060: ResourceBudget
static void test_resource_budget()
{
    runtime::ResourceBudget b;
    b.max_inflight_read_count = primitives::Count{4};
    b.max_buffer_bytes = primitives::Bytes{1048576};
    b.max_memory_pool_bytes = primitives::Bytes{16777216};
    assert(b.max_inflight_read_count.value == 4);
    std::cout << "PASS: resource_budget\n";
}

static void test_resource_budget_event_kind()
{
    runtime::ResourceBudgetEventKind k = runtime::ResourceBudgetEventKind::ShrinkStarted;
    assert(k == runtime::ResourceBudgetEventKind::ShrinkStarted);
    std::cout << "PASS: resource_budget_event_kind\n";
}

int main()
{
    test_recovery_decision_kind();
    test_recovery_context();
    test_recovery_budget();
    test_authorized_fallback();
    test_playback_failure_kind();
    test_playback_failure();
    test_failure_taxonomy_policy();
    test_drm_failure_policy();
    test_quarantined_reader_record();
    test_shutdown_quiescence_policy();
    test_event_delivery_class();
    test_event_bus_policy();
    test_session_event();
    test_control_event_queue_policy();
    test_critical_event_queue_kind();
    test_stable_reason_code();
    test_safe_diagnostic_string();
    test_fixed_window_histogram();
    test_session_diagnostics();
    test_low_speed_policy();
    test_reader_stall_policy();
    test_flush_scope_buffer_only();
    test_flush_scope_reader_in_flight();
    test_flush_scope_stream_revision();
    test_flush_scope_full_session();
    test_resource_budget();
    test_resource_budget_event_kind();
    std::cout << "ALL PASS: p4_recovery_events_diagnostics_tests (27 tests)\n";
    return 0;
}
