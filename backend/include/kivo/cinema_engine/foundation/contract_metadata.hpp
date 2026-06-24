#pragma once

#include <optional>
#include <string>
#include <utility>

namespace kivo::cinema_engine {

inline constexpr const char* kCoreSchemaVersion = "1.1";
inline constexpr const char* kProducerVersion = "kivo-cinema-engine-p1";

struct ContractMetadata {
  std::string schema_version{kCoreSchemaVersion};
  std::string producer_version{kProducerVersion};
  std::optional<std::string> replay_version{"1.1"};
  std::string trace_id;
  std::optional<std::string> session_id;
};

inline ContractMetadata make_contract_metadata(std::string trace_id,
                                               std::optional<std::string> session_id = std::nullopt) {
  ContractMetadata metadata;
  metadata.trace_id = std::move(trace_id);
  metadata.session_id = std::move(session_id);
  return metadata;
}

}  // namespace kivo::cinema_engine
