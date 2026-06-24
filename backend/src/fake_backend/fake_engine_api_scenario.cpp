#include "kivo/cinema_engine/fake_backend/fake_engine_api_scenario.hpp"

#include "kivo/cinema_engine/decision_ledger_core/decision_ledger_store.hpp"
#include "kivo/cinema_engine/engine_api/command_queue.hpp"
#include "kivo/cinema_engine/engine_api/engine_command.hpp"
#include "kivo/cinema_engine/engine_api/event_bus.hpp"
#include "kivo/cinema_engine/engine_api/state_snapshot_store.hpp"
#include "kivo/cinema_engine/session_core/session_store.hpp"

namespace kivo::cinema_engine {

namespace {

EngineEvent event_from_ledger_entry(const DecisionLedgerEntry& entry) {
  EngineEvent event;
  event.metadata = entry.metadata;
  event.event_id = "event." + entry.entry_id;
  event.producer_module = entry.actor;
  event.sequence_number = entry.sequence_number;
  event.generation = entry.generation.value_or(0);
  event.event_kind = entry.rationale;
  return event;
}

}  // namespace

FakeEngineApiScenarioResult run_fake_engine_api_scenario() {
  FakeEngineApiScenarioResult result;

  CommandQueue command_queue;
  EventBus event_bus;
  StateSnapshotStore snapshot_store;
  SessionStore session_store;
  DecisionLedgerStore ledger_store;

  const auto subscription_id = event_bus.subscribe();

  EngineCommand command;
  command.metadata = make_contract_metadata("trace.fake.engine-api.0001");
  command.command_id = "command.fake.open.0001";
  command.kind = EngineCommandKind::Open;
  command.payload_ref = "intent.fake.open";
  result.command_accepted = command_queue.submit(command);

  const auto popped = command_queue.try_pop();
  if (!popped.has_value()) {
    return result;
  }

  result.session_scenario = build_fake_direct_play_session();
  session_store.create(result.session_scenario.session);
  for (const auto& entry : result.session_scenario.decision_ledger) {
    ledger_store.append(entry);
    event_bus.publish(event_from_ledger_entry(entry));
  }

  EngineStateSnapshot snapshot;
  snapshot.metadata = make_contract_metadata(result.session_scenario.session.trace_root_id, result.session_scenario.session.session_id);
  snapshot.snapshot_id = "snapshot.fake.engine-api.0001";
  snapshot.engine_lifecycle_state = "Running";
  snapshot.playback_session_state = result.session_scenario.session.session_id;
  snapshot.playback_state = "Playing";
  snapshot.graph_runtime_summary = result.session_scenario.session.graph_id;
  snapshot.created_at = result.session_scenario.session.created_at;
  snapshot_store.publish(snapshot);

  result.state_snapshot_published = snapshot_store.has_snapshot();
  result.latest_snapshot = snapshot_store.latest().value_or(EngineStateSnapshot{});
  result.observed_events = event_bus.poll(subscription_id);
  result.replay_manifest = build_replay_manifest(
      result.session_scenario.session,
      ledger_store.entries_for_session(result.session_scenario.session.session_id));

  return result;
}

}  // namespace kivo::cinema_engine
