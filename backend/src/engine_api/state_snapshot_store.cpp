#include "kivo/cinema_engine/engine_api/state_snapshot_store.hpp"

#include <utility>

namespace kivo::cinema_engine {

void StateSnapshotStore::publish(EngineStateSnapshot snapshot) {
  latest_ = std::move(snapshot);
}

std::optional<EngineStateSnapshot> StateSnapshotStore::latest() const {
  return latest_;
}

bool StateSnapshotStore::has_snapshot() const {
  return latest_.has_value();
}

}  // namespace kivo::cinema_engine
