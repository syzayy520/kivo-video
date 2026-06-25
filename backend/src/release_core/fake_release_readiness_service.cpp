#include "kivo/cinema_engine/release_core/fake_release_readiness_service.hpp"

namespace kivo::cinema_engine {

ReadinessCheck FakeReleaseReadinessService::build_passing_check() {
    ReadinessCheck c;
    c.metadata.schema_version = "2.0";
    c.check_id = "build-pass-001";
    c.check_area = "build";
    c.check_status = ReadinessCheckStatus::Pass;
    c.blocking = true;
    c.evidence = "Build compiles without warnings";
    c.last_verified_iso = "2026-06-25T00:00:00Z";
    return c;
}

ReadinessCheck FakeReleaseReadinessService::build_failing_check() {
    ReadinessCheck c;
    c.metadata.schema_version = "2.0";
    c.check_id = "security-fail-001";
    c.check_area = "security";
    c.check_status = ReadinessCheckStatus::Fail;
    c.blocking = true;
    c.evidence = "Vulnerability scan found 2 critical issues";
    c.last_verified_iso = "2026-06-25T00:00:00Z";
    return c;
}

ReadinessCheck FakeReleaseReadinessService::build_warning_check() {
    ReadinessCheck c;
    c.metadata.schema_version = "2.0";
    c.check_id = "perf-warn-001";
    c.check_area = "performance";
    c.check_status = ReadinessCheckStatus::Warning;
    c.blocking = false;
    c.evidence = "Frame drop rate slightly above threshold";
    c.last_verified_iso = "2026-06-25T00:00:00Z";
    return c;
}

ReadinessCheck FakeReleaseReadinessService::build_skipped_check() {
    ReadinessCheck c;
    c.metadata.schema_version = "2.0";
    c.check_id = "compat-skip-001";
    c.check_area = "compatibility";
    c.check_status = ReadinessCheckStatus::Skipped;
    c.blocking = false;
    c.evidence = "Compatibility test skipped due to missing device";
    c.last_verified_iso = "2026-06-25T00:00:00Z";
    return c;
}

ReadinessMatrix FakeReleaseReadinessService::build_ready_matrix() {
    ReadinessMatrix m;
    m.metadata.schema_version = "2.0";
    m.matrix_id = "release-ready-001";
    m.version = "1.0.0";
    m.total_checks = 10;
    m.passed_checks = 10;
    m.failed_checks = 0;
    m.warning_checks = 0;
    m.blocking_failures = 0;
    m.release_ready = true;
    return m;
}

ReadinessMatrix FakeReleaseReadinessService::build_not_ready_matrix() {
    ReadinessMatrix m;
    m.metadata.schema_version = "2.0";
    m.matrix_id = "release-not-ready-001";
    m.version = "1.0.0";
    m.total_checks = 10;
    m.passed_checks = 7;
    m.failed_checks = 3;
    m.warning_checks = 0;
    m.blocking_failures = 3;
    m.release_ready = false;
    return m;
}

ReadinessMatrix FakeReleaseReadinessService::build_conditional_matrix() {
    ReadinessMatrix m;
    m.metadata.schema_version = "2.0";
    m.matrix_id = "release-conditional-001";
    m.version = "1.0.0";
    m.total_checks = 10;
    m.passed_checks = 8;
    m.failed_checks = 1;
    m.warning_checks = 1;
    m.blocking_failures = 1;
    m.release_ready = false;
    return m;
}

ReleaseGate FakeReleaseReadinessService::build_ready_gate() {
    ReleaseGate g;
    g.metadata.schema_version = "2.0";
    g.gate_id = "gate-ready-001";
    g.gate_decision = GateDecision::Ready;
    g.blocking_items = 0;
    g.non_blocking_warnings = 0;
    g.gate_message = "All checks passed, release is ready";
    g.next_action = "Proceed with release";
    return g;
}

ReleaseGate FakeReleaseReadinessService::build_not_ready_gate() {
    ReleaseGate g;
    g.metadata.schema_version = "2.0";
    g.gate_id = "gate-not-ready-001";
    g.gate_decision = GateDecision::NotReady;
    g.blocking_items = 3;
    g.non_blocking_warnings = 1;
    g.gate_message = "3 blocking issues prevent release";
    g.next_action = "Fix blocking issues before release";
    return g;
}

ReleaseGate FakeReleaseReadinessService::build_conditional_gate() {
    ReleaseGate g;
    g.metadata.schema_version = "2.0";
    g.gate_id = "gate-conditional-001";
    g.gate_decision = GateDecision::ConditionalReady;
    g.blocking_items = 1;
    g.non_blocking_warnings = 2;
    g.gate_message = "Release conditionally ready with 1 blocking item";
    g.next_action = "Review blocking item or grant exception";
    return g;
}

ReadinessCheck FakeReleaseReadinessService::build_build_area_checks() {
    ReadinessCheck c;
    c.metadata.schema_version = "2.0";
    c.check_id = "build-area-001";
    c.check_area = "build";
    c.check_status = ReadinessCheckStatus::Pass;
    c.blocking = true;
    c.evidence = "MSVC/Ninja build passes with 0 errors";
    c.last_verified_iso = "2026-06-25T00:00:00Z";
    return c;
}

ReadinessCheck FakeReleaseReadinessService::build_governance_area_checks() {
    ReadinessCheck c;
    c.metadata.schema_version = "2.0";
    c.check_id = "governance-area-001";
    c.check_area = "governance";
    c.check_status = ReadinessCheckStatus::Pass;
    c.blocking = true;
    c.evidence = "All governance gates pass";
    c.last_verified_iso = "2026-06-25T00:00:00Z";
    return c;
}

}  // namespace kivo::cinema_engine