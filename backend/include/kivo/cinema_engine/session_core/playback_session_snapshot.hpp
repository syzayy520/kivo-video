#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct PlaybackSessionSnapshot {
  ContractMetadata metadata;
  std::string snapshot_id;
  std::string session_id;
  std::string playback_session_state;
  std::optional<std::string> playback_state;
  std::optional<std::string> active_plan_id;
  std::optional<std::string> active_graph_id;
  std::string created_at;
};

}  // namespace kivo::cinema_engine

