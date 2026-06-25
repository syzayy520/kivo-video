#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/integration_core/fake_integration_gate_service.hpp"

using namespace kivo::cinema_engine;

void test_passing_gate_request() {
    auto request = FakeIntegrationGateService::build_passing_gate_request();
    assert(request.gate_id == "integration-pass-001");
    assert(request.subphases_to_evaluate.size() == 4);
    assert(request.strict_mode == false);
    assert(!request.timestamp.empty());
    std::cout << "PASS: test_passing_gate_request\n";
}

void test_failing_gate_request() {
    auto request = FakeIntegrationGateService::build_failing_gate_request();
    assert(request.gate_id == "integration-fail-001");
    assert(request.subphases_to_evaluate.size() == 3);
    assert(request.strict_mode == false);
    assert(!request.timestamp.empty());
    std::cout << "PASS: test_failing_gate_request\n";
}

void test_strict_mode_request() {
    auto request = FakeIntegrationGateService::build_strict_mode_request();
    assert(request.gate_id == "integration-strict-001");
    assert(request.subphases_to_evaluate.size() == 4);
    assert(request.strict_mode == true);
    assert(!request.timestamp.empty());
    std::cout << "PASS: test_strict_mode_request\n";
}

void test_ready_result() {
    auto result = FakeIntegrationGateService::build_ready_result();
    assert(result.gate_id == "integration-ready-001");
    assert(result.overall_decision == IntegrationDecision::Ready);
    assert(result.blocking_issues == 0);
    assert(result.warnings == 0);
    assert(result.subphase_results.size() == 4);
    assert(!result.summary_message.empty());
    std::cout << "PASS: test_ready_result\n";
}

void test_not_ready_result() {
    auto result = FakeIntegrationGateService::build_not_ready_result();
    assert(result.gate_id == "integration-not-ready-001");
    assert(result.overall_decision == IntegrationDecision::NotReady);
    assert(result.blocking_issues == 2);
    assert(result.warnings == 1);
    assert(result.subphase_results.size() == 4);
    assert(!result.summary_message.empty());
    std::cout << "PASS: test_not_ready_result\n";
}

void test_conditional_result() {
    auto result = FakeIntegrationGateService::build_conditional_result();
    assert(result.gate_id == "integration-conditional-001");
    assert(result.overall_decision == IntegrationDecision::ConditionalReady);
    assert(result.blocking_issues == 1);
    assert(result.warnings == 2);
    assert(result.subphase_results.size() == 4);
    assert(!result.summary_message.empty());
    std::cout << "PASS: test_conditional_result\n";
}

void test_partial_result() {
    auto result = FakeIntegrationGateService::build_partial_result();
    assert(result.gate_id == "integration-partial-001");
    assert(result.overall_decision == IntegrationDecision::NotReady);
    assert(result.blocking_issues == 1);
    assert(result.subphase_results.size() == 3);
    assert(!result.summary_message.empty());
    std::cout << "PASS: test_partial_result\n";
}

void test_p2_1_status() {
    auto status = FakeIntegrationGateService::build_p2_1_status();
    assert(status.subphase_id == "P2.1");
    assert(status.status == SubphaseStatusKind::Pass);
    assert(!status.evidence.empty());
    assert(!status.last_verified.empty());
    std::cout << "PASS: test_p2_1_status\n";
}

void test_p2_1_5_status() {
    auto status = FakeIntegrationGateService::build_p2_1_5_status();
    assert(status.subphase_id == "P2.1.5");
    assert(status.status == SubphaseStatusKind::Pass);
    assert(!status.evidence.empty());
    assert(!status.last_verified.empty());
    std::cout << "PASS: test_p2_1_5_status\n";
}

void test_p2_2_status() {
    auto status = FakeIntegrationGateService::build_p2_2_status();
    assert(status.subphase_id == "P2.2");
    assert(status.status == SubphaseStatusKind::Pass);
    assert(!status.evidence.empty());
    assert(!status.last_verified.empty());
    std::cout << "PASS: test_p2_2_status\n";
}

void test_p2_3_status() {
    auto status = FakeIntegrationGateService::build_p2_3_status();
    assert(status.subphase_id == "P2.3");
    assert(status.status == SubphaseStatusKind::Pass);
    assert(!status.evidence.empty());
    assert(!status.last_verified.empty());
    std::cout << "PASS: test_p2_3_status\n";
}

void test_default_policy() {
    auto policy = FakeIntegrationGateService::build_default_policy();
    assert(policy.policy_id == "default-integration-policy");
    assert(policy.required_subphases.size() == 4);
    assert(policy.allow_conditional == true);
    assert(policy.max_blocking_items == 0);
    assert(policy.require_evidence == true);
    std::cout << "PASS: test_default_policy\n";
}

int main() {
    test_passing_gate_request();
    test_failing_gate_request();
    test_strict_mode_request();
    test_ready_result();
    test_not_ready_result();
    test_conditional_result();
    test_partial_result();
    test_p2_1_status();
    test_p2_1_5_status();
    test_p2_2_status();
    test_p2_3_status();
    test_default_policy();
    std::cout << "ALL 12 INTEGRATION GATE TESTS PASSED\n";
    return 0;
}