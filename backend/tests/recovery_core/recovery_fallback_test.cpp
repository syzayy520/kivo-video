#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/recovery_core/fake_recovery_fallback_service.hpp"

using namespace kivo::cinema_engine;

void test_retry_action() {
    auto r = FakeRecoveryFallbackService::build_retry_action();
    assert(!r.action_id.empty());
    assert(r.action_kind == RecoveryActionKind::Retry);
    assert(r.max_retries == 3);
    assert(r.retry_delay_ms > 0);
}

void test_fallback_action() {
    auto r = FakeRecoveryFallbackService::build_fallback_action();
    assert(!r.action_id.empty());
    assert(r.action_kind == RecoveryActionKind::Fallback);
    assert(r.max_retries == 0);
}

void test_abort_action() {
    auto r = FakeRecoveryFallbackService::build_abort_action();
    assert(!r.action_id.empty());
    assert(r.action_kind == RecoveryActionKind::Abort);
    assert(r.trigger_error_severity == RecoveryTriggerSeverity::Fatal);
}

void test_restart_action() {
    auto r = FakeRecoveryFallbackService::build_restart_action();
    assert(!r.action_id.empty());
    assert(r.action_kind == RecoveryActionKind::RestartComponent);
    assert(r.max_retries == 2);
}

void test_source_unavailable_fallback() {
    auto f = FakeRecoveryFallbackService::build_source_unavailable_fallback();
    assert(!f.strategy_id.empty());
    assert(f.fallback_reason == FallbackReason::SourceUnavailable);
    assert(f.auto_apply == true);
}

void test_codec_unsupported_fallback() {
    auto f = FakeRecoveryFallbackService::build_codec_unsupported_fallback();
    assert(!f.strategy_id.empty());
    assert(f.fallback_reason == FallbackReason::CodecUnsupported);
    assert(f.estimated_quality_impact == QualityImpact::Moderate);
}

void test_network_timeout_fallback() {
    auto f = FakeRecoveryFallbackService::build_network_timeout_fallback();
    assert(!f.strategy_id.empty());
    assert(f.fallback_reason == FallbackReason::NetworkTimeout);
    assert(f.user_notification_required == true);
}

void test_green_budget() {
    auto b = FakeRecoveryFallbackService::build_green_budget();
    assert(!b.budget_id.empty());
    assert(b.current_failures == 0);
    assert(b.budget_exhausted == false);
}

void test_warning_budget() {
    auto b = FakeRecoveryFallbackService::build_warning_budget();
    assert(!b.budget_id.empty());
    assert(b.current_failures == 4);
    assert(b.max_failures_per_window == 5);
    assert(b.budget_exhausted == false);
}

void test_exhausted_budget() {
    auto b = FakeRecoveryFallbackService::build_exhausted_budget();
    assert(!b.budget_id.empty());
    assert(b.budget_exhausted == true);
    assert(b.current_failures >= b.max_failures_per_window);
}

void test_recovering_budget() {
    auto b = FakeRecoveryFallbackService::build_recovering_budget();
    assert(!b.budget_id.empty());
    assert(b.budget_exhausted == false);
    assert(b.current_failures > 0);
}

void test_cross_component_budget() {
    auto b = FakeRecoveryFallbackService::build_cross_component_budget();
    assert(!b.budget_id.empty());
    assert(b.component == "demux+decode");
    assert(b.window_size_seconds == 120);
}

int main() {
    std::cout << "Running recovery/fallback tests..." << std::endl;

    test_retry_action();
    test_fallback_action();
    test_abort_action();
    test_restart_action();
    test_source_unavailable_fallback();
    test_codec_unsupported_fallback();
    test_network_timeout_fallback();
    test_green_budget();
    test_warning_budget();
    test_exhausted_budget();
    test_recovering_budget();
    test_cross_component_budget();

    std::cout << "All 12 recovery/fallback tests passed." << std::endl;
    return 0;
}
