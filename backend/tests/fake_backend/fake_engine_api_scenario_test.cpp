#include <cassert>

#include "kivo/cinema_engine/fake_backend/fake_engine_api_scenario.hpp"

int fake_engine_api_scenario_test() {
  const auto result = kivo::cinema_engine::run_fake_engine_api_scenario();

  assert(result.command_accepted);
  assert(result.state_snapshot_published);
  assert(result.latest_snapshot.engine_lifecycle_state == "Running");
  assert(result.latest_snapshot.playback_state == "Playing");
  assert(result.observed_events.size() == result.session_scenario.decision_ledger.size());
  assert(result.replay_manifest.replayable);
  assert(result.replay_manifest.ordered_entry_ids.size() == result.session_scenario.decision_ledger.size());

  return 0;
}

