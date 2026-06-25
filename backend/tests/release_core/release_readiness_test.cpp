#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/release_core/fake_release_readiness_service.hpp"

using namespace kivo::cinema_engine;

void test_passing_check() {
    auto check = FakeReleaseReadinessService::build_passing_check();
    assert(check.check_id == "build-pass-001");
    assert(check.check_area == "build");
    assert(check.check_status == ReadinessCheckStatus::Pass);
    assert(check.blocking == true);
    assert(!check.evidence.empty());
    assert(!check.last_verified_iso.empty());
    std::cout << "PASS: test_passing_check\n";
}

void test_failing_check() {
    auto check = FakeReleaseReadinessService::build_failing_check();
    assert(check.check_id == "security-fail-001");
    assert(check.check_area == "security");
    assert(check.check_status == ReadinessCheckStatus::Fail);
    assert(check.blocking == true);
    assert(!check.evidence.empty());
    std::cout << "PASS: test_failing_check\n";
}

void test_warning_check() {
    auto check = FakeReleaseReadinessService::build_warning_check();
    assert(check.check_id == "perf-warn-001");
    assert(check.check_area == "performance");
    assert(check.check_status == ReadinessCheckStatus::Warning);
    assert(check.blocking == false);
    assert(!check.evidence.empty());
    std::cout << "PASS: test_warning_check\n";
}

void test_skipped_check() {
    auto check = FakeReleaseReadinessService::build_skipped_check();
    assert(check.check_id == "compat-skip-001");
    assert(check.check_area == "compatibility");
    assert(check.check_status == ReadinessCheckStatus::Skipped);
    assert(check.blocking == false);
    assert(!check.evidence.empty());
    std::cout << "PASS: test_skipped_check\n";
}

void test_ready_matrix() {
    auto matrix = FakeReleaseReadinessService::build_ready_matrix();
    assert(matrix.matrix_id == "release-ready-001");
    assert(matrix.version == "1.0.0");
    assert(matrix.total_checks == 10);
    assert(matrix.passed_checks == 10);
    assert(matrix.failed_checks == 0);
    assert(matrix.warning_checks == 0);
    assert(matrix.blocking_failures == 0);
    assert(matrix.release_ready == true);
    std::cout << "PASS: test_ready_matrix\n";
}

void test_not_ready_matrix() {
    auto matrix = FakeReleaseReadinessService::build_not_ready_matrix();
    assert(matrix.matrix_id == "release-not-ready-001");
    assert(matrix.version == "1.0.0");
    assert(matrix.total_checks == 10);
    assert(matrix.passed_checks == 7);
    assert(matrix.failed_checks == 3);
    assert(matrix.warning_checks == 0);
    assert(matrix.blocking_failures == 3);
    assert(matrix.release_ready == false);
    std::cout << "PASS: test_not_ready_matrix\n";
}

void test_conditional_matrix() {
    auto matrix = FakeReleaseReadinessService::build_conditional_matrix();
    assert(matrix.matrix_id == "release-conditional-001");
    assert(matrix.version == "1.0.0");
    assert(matrix.total_checks == 10);
    assert(matrix.passed_checks == 8);
    assert(matrix.failed_checks == 1);
    assert(matrix.warning_checks == 1);
    assert(matrix.blocking_failures == 1);
    assert(matrix.release_ready == false);
    std::cout << "PASS: test_conditional_matrix\n";
}

void test_ready_gate() {
    auto gate = FakeReleaseReadinessService::build_ready_gate();
    assert(gate.gate_id == "gate-ready-001");
    assert(gate.gate_decision == GateDecision::Ready);
    assert(gate.blocking_items == 0);
    assert(gate.non_blocking_warnings == 0);
    assert(!gate.gate_message.empty());
    assert(!gate.next_action.empty());
    std::cout << "PASS: test_ready_gate\n";
}

void test_not_ready_gate() {
    auto gate = FakeReleaseReadinessService::build_not_ready_gate();
    assert(gate.gate_id == "gate-not-ready-001");
    assert(gate.gate_decision == GateDecision::NotReady);
    assert(gate.blocking_items == 3);
    assert(gate.non_blocking_warnings == 1);
    assert(!gate.gate_message.empty());
    assert(!gate.next_action.empty());
    std::cout << "PASS: test_not_ready_gate\n";
}

void test_conditional_gate() {
    auto gate = FakeReleaseReadinessService::build_conditional_gate();
    assert(gate.gate_id == "gate-conditional-001");
    assert(gate.gate_decision == GateDecision::ConditionalReady);
    assert(gate.blocking_items == 1);
    assert(gate.non_blocking_warnings == 2);
    assert(!gate.gate_message.empty());
    assert(!gate.next_action.empty());
    std::cout << "PASS: test_conditional_gate\n";
}

void test_build_area_checks() {
    auto check = FakeReleaseReadinessService::build_build_area_checks();
    assert(check.check_id == "build-area-001");
    assert(check.check_area == "build");
    assert(check.check_status == ReadinessCheckStatus::Pass);
    assert(check.blocking == true);
    assert(!check.evidence.empty());
    std::cout << "PASS: test_build_area_checks\n";
}

void test_governance_area_checks() {
    auto check = FakeReleaseReadinessService::build_governance_area_checks();
    assert(check.check_id == "governance-area-001");
    assert(check.check_area == "governance");
    assert(check.check_status == ReadinessCheckStatus::Pass);
    assert(check.blocking == true);
    assert(!check.evidence.empty());
    std::cout << "PASS: test_governance_area_checks\n";
}

int main() {
    test_passing_check();
    test_failing_check();
    test_warning_check();
    test_skipped_check();
    test_ready_matrix();
    test_not_ready_matrix();
    test_conditional_matrix();
    test_ready_gate();
    test_not_ready_gate();
    test_conditional_gate();
    test_build_area_checks();
    test_governance_area_checks();
    std::cout << "ALL 12 RELEASE READINESS TESTS PASSED\n";
    return 0;
}