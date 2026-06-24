#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct EngineError {
  ContractMetadata metadata;
  std::string domain;
  std::string code;
  std::string severity;
  std::string user_message_key;
  std::string technical_message;
  std::optional<std::string> source_id;
  std::optional<std::string> media_id;
  std::optional<std::string> session_id;
  std::string recoverability;
  bool fallback_available{false};
};

}  // namespace kivo::cinema_engine

