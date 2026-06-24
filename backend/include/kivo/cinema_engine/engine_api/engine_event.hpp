#pragma once

#include <cstdint>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct EngineEvent {
  ContractMetadata metadata;
  std::string event_id;
  std::string producer_module;
  std::uint64_t sequence_number{0};
  std::uint64_t generation{0};
  std::string causation_id;
  std::string correlation_id;
  std::string event_kind;
};

}  // namespace kivo::cinema_engine

