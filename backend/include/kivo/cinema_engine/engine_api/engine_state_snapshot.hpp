#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct EngineStateSnapshot {
  ContractMetadata metadata;
  std::string snapshot_id;
  std::string engine_lifecycle_state;
  std::optional<std::string> playback_session_state;
  std::optional<std::string> playback_state;
  std::optional<std::string> graph_runtime_summary;
  std::optional<std::string> library_state_summary;
  std::optional<std::string> error_summary;
  std::string created_at;
};

}  // namespace kivo::cinema_engine

