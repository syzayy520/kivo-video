#pragma once

#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

enum class EngineCommandKind {
  Open,
  Play,
  Pause,
  Seek,
  Recover,
  Close
};

struct EngineCommand {
  ContractMetadata metadata;
  std::string command_id;
  EngineCommandKind kind{EngineCommandKind::Open};
  std::string payload_ref;
};

}  // namespace kivo::cinema_engine

