#include "kivo/cinema_engine/recovery_core/fake_recovery_fallback_service.hpp"

namespace kivo::cinema_engine {

RecoveryAction FakeRecoveryFallbackService::build_retry_action() {
    RecoveryAction r;
    r.metadata.schema_version = "2.0";
    r.action_id = "retry-decode-001";
    r.action_kind = RecoveryActionKind::Retry;
    r.trigger_error_domain = "decode";
    r.trigger_error_severity = RecoveryTriggerSeverity::Error;
    r.max_retries = 3;
    r.retry_delay_ms = 1000;
    r.user_visible_hint = "Retrying decode after transient error";
    return r;
}

RecoveryAction FakeRecoveryFallbackService::build_fallback_action() {
    RecoveryAction r;
    r.metadata.schema_version = "2.0";
    r.action_id = "fallback-sw-decode-001";
    r.action_kind = RecoveryActionKind::Fallback;
    r.trigger_error_domain = "decode";
    r.trigger_error_severity = RecoveryTriggerSeverity::Error;
    r.max_retries = 0;
    r.retry_delay_ms = 0;
    r.user_visible_hint = "Falling back to software decode";
    return r;
}

RecoveryAction FakeRecoveryFallbackService::build_abort_action() {
    RecoveryAction r;
    r.metadata.schema_version = "2.0";
    r.action_id = "abort-playback-001";
    r.action_kind = RecoveryActionKind::Abort;
    r.trigger_error_domain = "source";
    r.trigger_error_severity = RecoveryTriggerSeverity::Fatal;
    r.max_retries = 0;
    r.retry_delay_ms = 0;
    r.user_visible_hint = "Playback aborted due to unrecoverable error";
    return r;
}

RecoveryAction FakeRecoveryFallbackService::build_restart_action() {
    RecoveryAction r;
    r.metadata.schema_version = "2.0";
    r.action_id = "restart-renderer-001";
    r.action_kind = RecoveryActionKind::RestartComponent;
    r.trigger_error_domain = "render";
    r.trigger_error_severity = RecoveryTriggerSeverity::Error;
    r.max_retries = 2;
    r.retry_delay_ms = 500;
    r.user_visible_hint = "Restarting video renderer";
    return r;
}

FallbackStrategy FakeRecoveryFallbackService::build_source_unavailable_fallback() {
    FallbackStrategy f;
    f.metadata.schema_version = "2.0";
    f.strategy_id = "source-unavailable-001";
    f.primary_path = "webdav://nas.local/media/movie.mkv";
    f.fallback_path = "local://cache/movie_partial.mkv";
    f.fallback_reason = FallbackReason::SourceUnavailable;
    f.estimated_quality_impact = QualityImpact::Minor;
    f.auto_apply = true;
    f.user_notification_required = false;
    return f;
}

FallbackStrategy FakeRecoveryFallbackService::build_codec_unsupported_fallback() {
    FallbackStrategy f;
    f.metadata.schema_version = "2.0";
    f.strategy_id = "codec-unsupported-001";
    f.primary_path = "hw-decode://d3d11/hevc";
    f.fallback_path = "sw-decode://ffmpeg/hevc";
    f.fallback_reason = FallbackReason::CodecUnsupported;
    f.estimated_quality_impact = QualityImpact::Moderate;
    f.auto_apply = true;
    f.user_notification_required = true;
    return f;
}

FallbackStrategy FakeRecoveryFallbackService::build_network_timeout_fallback() {
    FallbackStrategy f;
    f.metadata.schema_version = "2.0";
    f.strategy_id = "network-timeout-001";
    f.primary_path = "emby://server/stream/12345";
    f.fallback_path = "emby://server/transcode/12345";
    f.fallback_reason = FallbackReason::NetworkTimeout;
    f.estimated_quality_impact = QualityImpact::Significant;
    f.auto_apply = false;
    f.user_notification_required = true;
    return f;
}

FailureBudget FakeRecoveryFallbackService::build_green_budget() {
    FailureBudget b;
    b.metadata.schema_version = "2.0";
    b.budget_id = "budget-green-001";
    b.component = "decode";
    b.window_size_seconds = 60;
    b.max_failures_per_window = 5;
    b.current_failures = 0;
    b.budget_exhausted = false;
    b.recovery_cooldown_ms = 30000;
    return b;
}

FailureBudget FakeRecoveryFallbackService::build_warning_budget() {
    FailureBudget b;
    b.metadata.schema_version = "2.0";
    b.budget_id = "budget-warning-001";
    b.component = "demux";
    b.window_size_seconds = 60;
    b.max_failures_per_window = 5;
    b.current_failures = 4;
    b.budget_exhausted = false;
    b.recovery_cooldown_ms = 30000;
    return b;
}

FailureBudget FakeRecoveryFallbackService::build_exhausted_budget() {
    FailureBudget b;
    b.metadata.schema_version = "2.0";
    b.budget_id = "budget-exhausted-001";
    b.component = "render";
    b.window_size_seconds = 60;
    b.max_failures_per_window = 3;
    b.current_failures = 3;
    b.budget_exhausted = true;
    b.recovery_cooldown_ms = 60000;
    return b;
}

FailureBudget FakeRecoveryFallbackService::build_recovering_budget() {
    FailureBudget b;
    b.metadata.schema_version = "2.0";
    b.budget_id = "budget-recovering-001";
    b.component = "audio";
    b.window_size_seconds = 60;
    b.max_failures_per_window = 5;
    b.current_failures = 2;
    b.budget_exhausted = false;
    b.recovery_cooldown_ms = 30000;
    return b;
}

FailureBudget FakeRecoveryFallbackService::build_cross_component_budget() {
    FailureBudget b;
    b.metadata.schema_version = "2.0";
    b.budget_id = "budget-cross-001";
    b.component = "demux+decode";
    b.window_size_seconds = 120;
    b.max_failures_per_window = 10;
    b.current_failures = 5;
    b.budget_exhausted = false;
    b.recovery_cooldown_ms = 45000;
    return b;
}

}  // namespace kivo::cinema_engine
