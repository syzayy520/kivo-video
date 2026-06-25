#pragma once

#include "kivo/cinema_engine/recovery_core/recovery_action.hpp"
#include "kivo/cinema_engine/recovery_core/fallback_strategy.hpp"
#include "kivo/cinema_engine/recovery_core/failure_budget.hpp"

namespace kivo::cinema_engine {

// Fake service for recovery and fallback runtime testing.
struct FakeRecoveryFallbackService {
    // Recovery action builders
    static RecoveryAction build_retry_action();
    static RecoveryAction build_fallback_action();
    static RecoveryAction build_abort_action();
    static RecoveryAction build_restart_action();

    // Fallback strategy builders
    static FallbackStrategy build_source_unavailable_fallback();
    static FallbackStrategy build_codec_unsupported_fallback();
    static FallbackStrategy build_network_timeout_fallback();

    // Failure budget builders
    static FailureBudget build_green_budget();
    static FailureBudget build_warning_budget();
    static FailureBudget build_exhausted_budget();
    static FailureBudget build_recovering_budget();
    static FailureBudget build_cross_component_budget();
};

}  // namespace kivo::cinema_engine
