#include <cassert>

#include "kivo/cinema_engine/fake_backend/fake_session_scenario.hpp"
#include "kivo/cinema_engine/foundation/contract_registry.hpp"

int fake_engine_api_scenario_test();

int main() {
  fake_engine_api_scenario_test();

  const auto scenario = kivo::cinema_engine::build_fake_direct_play_session();

  assert(kivo::cinema_engine::registry_contains_contract("PlaybackPlanSchema"));
  assert(kivo::cinema_engine::registry_contains_contract("RemoteObjectIdentity"));
  assert(kivo::cinema_engine::registry_contains_contract("ColorOutputMatrix"));
  assert(kivo::cinema_engine::fake_session_is_replayable(scenario));
  assert(scenario.cache_extent.remote_object_identity_ref == scenario.remote_object_identity.remote_object_identity_id);
  assert(scenario.playback_plan.feature_gate_snapshot_ref == scenario.feature_gate_snapshot.feature_gate_snapshot_id);
  assert(scenario.media_experience_snapshot.hdr_badge_claim_ref == scenario.compliance_gate_result.compliance_gate_id);
  assert(scenario.audio_output_decision.output_mode == "passthrough");

  return 0;
}
