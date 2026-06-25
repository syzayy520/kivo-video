#include "kivo/cinema_engine/integration_core/fake_integration_gate_service.hpp"

namespace kivo::cinema_engine {

IntegrationGateRequest FakeIntegrationGateService::build_passing_gate_request() {
    IntegrationGateRequest r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-pass-001";
    r.subphases_to_evaluate = {"P2.1", "P2.1.5", "P2.2", "P2.3"};
    r.strict_mode = false;
    r.timestamp = "2026-06-25T00:00:00Z";
    return r;
}

IntegrationGateRequest FakeIntegrationGateService::build_failing_gate_request() {
    IntegrationGateRequest r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-fail-001";
    r.subphases_to_evaluate = {"P2.1", "P2.1.5", "P2.2"};
    r.strict_mode = false;
    r.timestamp = "2026-06-25T00:00:00Z";
    return r;
}

IntegrationGateRequest FakeIntegrationGateService::build_strict_mode_request() {
    IntegrationGateRequest r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-strict-001";
    r.subphases_to_evaluate = {"P2.1", "P2.1.5", "P2.2", "P2.3"};
    r.strict_mode = true;
    r.timestamp = "2026-06-25T00:00:00Z";
    return r;
}

IntegrationGateResult FakeIntegrationGateService::build_ready_result() {
    IntegrationGateResult r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-ready-001";
    r.overall_decision = IntegrationDecision::Ready;
    r.blocking_issues = 0;
    r.warnings = 0;
    r.summary_message = "All P2 sub-phases passed, integration ready";
    r.subphase_results["P2.1"] = build_p2_1_status();
    r.subphase_results["P2.1.5"] = build_p2_1_5_status();
    r.subphase_results["P2.2"] = build_p2_2_status();
    r.subphase_results["P2.3"] = build_p2_3_status();
    return r;
}

IntegrationGateResult FakeIntegrationGateService::build_not_ready_result() {
    IntegrationGateResult r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-not-ready-001";
    r.overall_decision = IntegrationDecision::NotReady;
    r.blocking_issues = 2;
    r.warnings = 1;
    r.summary_message = "2 blocking issues prevent integration";
    r.subphase_results["P2.1"] = build_p2_1_status();
    r.subphase_results["P2.1.5"] = build_p2_1_5_status();
    r.subphase_results["P2.2"] = build_p2_2_status();
    r.subphase_results["P2.3"] = build_p2_3_status();
    return r;
}

IntegrationGateResult FakeIntegrationGateService::build_conditional_result() {
    IntegrationGateResult r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-conditional-001";
    r.overall_decision = IntegrationDecision::ConditionalReady;
    r.blocking_issues = 1;
    r.warnings = 2;
    r.summary_message = "Integration conditionally ready with 1 blocking item";
    r.subphase_results["P2.1"] = build_p2_1_status();
    r.subphase_results["P2.1.5"] = build_p2_1_5_status();
    r.subphase_results["P2.2"] = build_p2_2_status();
    r.subphase_results["P2.3"] = build_p2_3_status();
    return r;
}

IntegrationGateResult FakeIntegrationGateService::build_partial_result() {
    IntegrationGateResult r;
    r.metadata.schema_version = "2.0";
    r.gate_id = "integration-partial-001";
    r.overall_decision = IntegrationDecision::NotReady;
    r.blocking_issues = 1;
    r.warnings = 0;
    r.summary_message = "Missing P2.3 sub-phase evaluation";
    r.subphase_results["P2.1"] = build_p2_1_status();
    r.subphase_results["P2.1.5"] = build_p2_1_5_status();
    r.subphase_results["P2.2"] = build_p2_2_status();
    return r;
}

SubphaseStatus FakeIntegrationGateService::build_p2_1_status() {
    SubphaseStatus s;
    s.metadata.schema_version = "2.0";
    s.subphase_id = "P2.1";
    s.status = SubphaseStatusKind::Pass;
    s.evidence = "Local file playback gate: 14/14 tests pass";
    s.last_verified = "2026-06-25T00:00:00Z";
    return s;
}

SubphaseStatus FakeIntegrationGateService::build_p2_1_5_status() {
    SubphaseStatus s;
    s.metadata.schema_version = "2.0";
    s.subphase_id = "P2.1.5";
    s.status = SubphaseStatusKind::Pass;
    s.evidence = "BDMV playback gate: 5/5 tests pass";
    s.last_verified = "2026-06-25T00:00:00Z";
    return s;
}

SubphaseStatus FakeIntegrationGateService::build_p2_2_status() {
    SubphaseStatus s;
    s.metadata.schema_version = "2.0";
    s.subphase_id = "P2.2";
    s.status = SubphaseStatusKind::Pass;
    s.evidence = "Remote direct play gate: 10/10 tests pass";
    s.last_verified = "2026-06-25T00:00:00Z";
    return s;
}

SubphaseStatus FakeIntegrationGateService::build_p2_3_status() {
    SubphaseStatus s;
    s.metadata.schema_version = "2.0";
    s.subphase_id = "P2.3";
    s.status = SubphaseStatusKind::Pass;
    s.evidence = "Release readiness matrix: 12/12 tests pass";
    s.last_verified = "2026-06-25T00:00:00Z";
    return s;
}

IntegrationGatePolicy FakeIntegrationGateService::build_default_policy() {
    IntegrationGatePolicy p;
    p.metadata.schema_version = "2.0";
    p.policy_id = "default-integration-policy";
    p.required_subphases = {"P2.1", "P2.1.5", "P2.2", "P2.3"};
    p.allow_conditional = true;
    p.max_blocking_items = 0;
    p.require_evidence = true;
    return p;
}

}  // namespace kivo::cinema_engine