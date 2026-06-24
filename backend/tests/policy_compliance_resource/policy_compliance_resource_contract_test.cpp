#include <cassert>

#include "kivo/cinema_engine/compliance_core/fake_compliance_service.hpp"
#include "kivo/cinema_engine/feature_gate_core/fake_feature_gate_service.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/policy_core/fake_policy_service.hpp"
#include "kivo/cinema_engine/resource_core/fake_resource_budget_service.hpp"

int main() {
  const auto metadata = kivo::cinema_engine::make_contract_metadata("trace.fake.policy-compliance-resource", "session.fake.0001");

  // Test ResourceBudgetResult
  const auto resource_budget = kivo::cinema_engine::build_fake_resource_budget_result(
      metadata,
      "cpu.fake.80%",
      "gpu.fake.60%",
      "memory.fake.4gb",
      "network.fake.100mbps");
  assert(resource_budget.metadata.schema_version == "1.1");
  assert(resource_budget.resource_budget_id == "resource.fake.budget.0001");
  assert(resource_budget.cpu_budget == "cpu.fake.80%");
  assert(resource_budget.gpu_budget == "gpu.fake.60%");
  assert(resource_budget.memory_budget == "memory.fake.4gb");
  assert(resource_budget.network_budget == "network.fake.100mbps");
  assert(resource_budget.pressure_risks.size() == 3);
  assert(resource_budget.playback_allowed == true);

  // Test PolicyDecision
  const auto policy_decision = kivo::cinema_engine::build_fake_policy_decision(
      metadata,
      "allow",
      {"capability.met", "compliance.passed"},
  "Playback allowed based on current policy evaluation.");
  assert(policy_decision.metadata.schema_version == "1.1");
  assert(policy_decision.policy_decision_id == "policy.fake.decision.0001");
  assert(policy_decision.decision == "allow");
  assert(policy_decision.reasons.size() == 2);
  assert(policy_decision.user_visible_summary == "Playback allowed based on current policy evaluation.");

  // Test ComplianceGateResult
  const auto compliance_gate = kivo::cinema_engine::build_fake_compliance_gate_result(
      metadata,
      "generic_playback",
      {"playback", "seek", "pause"},
      {"drm.required"});
  assert(compliance_gate.metadata.schema_version == "1.1");
  assert(compliance_gate.compliance_gate_id == "compliance.fake.gate.0001");
  assert(compliance_gate.claim_level == "generic_playback");
  assert(compliance_gate.allowed_claims.size() == 3);
  assert(compliance_gate.forbidden_claim_reasons.size() == 1);
  assert(compliance_gate.certification_state == "certified");
  assert(compliance_gate.fail_closed == false);

  // Test FeatureGateSnapshot
  const auto feature_gate = kivo::cinema_engine::build_fake_feature_gate_snapshot(
      metadata,
      {"hdr10", "dolby_vision", "atmos"},
      {"legacy_codec"},
      {"hdr10_plus", "av1"});
  assert(feature_gate.metadata.schema_version == "1.1");
  assert(feature_gate.feature_gate_snapshot_id == "feature_gate.fake.snapshot.0001");
  assert(feature_gate.enabled_features.size() == 3);
  assert(feature_gate.disabled_features.size() == 1);
  assert(feature_gate.experimental_features.size() == 2);
  assert(feature_gate.compliance_limited_features.size() == 2);
  assert(feature_gate.reason == "fake.service.default");

  // Test fail_closed compliance gate
  kivo::cinema_engine::ComplianceGateResult fail_closed_gate;
  fail_closed_gate.metadata = metadata;
  fail_closed_gate.fail_closed = true;
  assert(fail_closed_gate.fail_closed == true);

  return 0;
}
