#include <cassert>

#include "kivo/cinema_engine/engine_api/cancellation_token.hpp"
#include "kivo/cinema_engine/engine_api/command_queue.hpp"
#include "kivo/cinema_engine/engine_api/event_bus.hpp"
#include "kivo/cinema_engine/engine_api/state_snapshot_store.hpp"

int main() {
  kivo::cinema_engine::CommandQueue command_queue;
  kivo::cinema_engine::EngineCommand open_command;
  open_command.command_id = "command.open.fake.0001";
  open_command.kind = kivo::cinema_engine::EngineCommandKind::Open;
  open_command.payload_ref = "intent.fake.open";
  assert(command_queue.submit(open_command));
  assert(command_queue.size() == 1);
  const auto popped_command = command_queue.try_pop();
  assert(popped_command.has_value());
  assert(popped_command->command_id == open_command.command_id);
  assert(command_queue.empty());

  kivo::cinema_engine::EventBus event_bus;
  const auto subscription_id = event_bus.subscribe();
  kivo::cinema_engine::EngineEvent event;
  event.event_id = "event.fake.0001";
  event.sequence_number = 1;
  event.generation = 1;
  event.event_kind = "CommandAccepted";
  event_bus.publish(event);
  const auto events = event_bus.poll(subscription_id);
  assert(events.size() == 1);
  assert(events.front().sequence_number == 1);
  assert(event_bus.poll(subscription_id).empty());

  kivo::cinema_engine::StateSnapshotStore snapshot_store;
  kivo::cinema_engine::EngineStateSnapshot snapshot;
  snapshot.snapshot_id = "snapshot.fake.0001";
  snapshot.engine_lifecycle_state = "Running";
  snapshot.created_at = "2026-06-24T00:00:00Z";
  snapshot_store.publish(snapshot);
  assert(snapshot_store.has_snapshot());
  assert(snapshot_store.latest()->engine_lifecycle_state == "Running");

  kivo::cinema_engine::CancellationSource cancellation_source;
  const auto token = cancellation_source.token();
  assert(!token.is_cancellation_requested());
  cancellation_source.request_cancellation();
  assert(token.is_cancellation_requested());

  return 0;
}

