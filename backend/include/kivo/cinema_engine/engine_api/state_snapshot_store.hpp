#pragma once

#include <optional>

#include "kivo/cinema_engine/engine_api/engine_state_snapshot.hpp"

namespace kivo::cinema_engine {

class StateSnapshotStore {
 public:
  void publish(EngineStateSnapshot snapshot);
  std::optional<EngineStateSnapshot> latest() const;
  bool has_snapshot() const;

 private:
  std::optional<EngineStateSnapshot> latest_;
};

}  // namespace kivo::cinema_engine

