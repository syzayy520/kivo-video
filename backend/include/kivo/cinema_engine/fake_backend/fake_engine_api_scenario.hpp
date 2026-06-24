#pragma once

#include <vector>

#include "kivo/cinema_engine/decision_ledger_core/replay_manifest.hpp"
#include "kivo/cinema_engine/engine_api/engine_event.hpp"
#include "kivo/cinema_engine/engine_api/engine_state_snapshot.hpp"
#include "kivo/cinema_engine/fake_backend/fake_session_scenario.hpp"

namespace kivo::cinema_engine {

struct FakeEngineApiScenarioResult {
  FakeSessionScenario session_scenario;
  ReplayManifest replay_manifest;
  std::vector<EngineEvent> observed_events;
  EngineStateSnapshot latest_snapshot;
  bool command_accepted{false};
  bool state_snapshot_published{false};
};

FakeEngineApiScenarioResult run_fake_engine_api_scenario();

}  // namespace kivo::cinema_engine

