#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct SourcePlayableObject {
  ContractMetadata metadata;
  std::string source_id;
  std::string source_kind;
  std::string playable_object_id;
  std::string identity;
  std::string access_policy;
  std::optional<std::string> credential_handle_ref;
  std::string lifetime;
  std::string refresh_policy;
  std::string capability_summary;
  std::optional<std::string> remote_object_identity_ref;
};

}  // namespace kivo::cinema_engine

