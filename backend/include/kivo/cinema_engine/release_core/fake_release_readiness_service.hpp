#pragma once

#include "kivo/cinema_engine/release_core/readiness_check.hpp"
#include "kivo/cinema_engine/release_core/readiness_matrix.hpp"
#include "kivo/cinema_engine/release_core/release_gate.hpp"

namespace kivo::cinema_engine {

// Fake service for release readiness matrix testing.
struct FakeReleaseReadinessService {
    // Readiness check builders
    static ReadinessCheck build_passing_check();
    static ReadinessCheck build_failing_check();
    static ReadinessCheck build_warning_check();
    static ReadinessCheck build_skipped_check();

    // Readiness matrix builders
    static ReadinessMatrix build_ready_matrix();
    static ReadinessMatrix build_not_ready_matrix();
    static ReadinessMatrix build_conditional_matrix();

    // Release gate builders
    static ReleaseGate build_ready_gate();
    static ReleaseGate build_not_ready_gate();
    static ReleaseGate build_conditional_gate();

    // Area-specific check builders
    static ReadinessCheck build_build_area_checks();
    static ReadinessCheck build_governance_area_checks();
};

}  // namespace kivo::cinema_engine
