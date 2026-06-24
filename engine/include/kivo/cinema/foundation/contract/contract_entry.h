#pragma once

#include <string>
#include <vector>

#include "kivo/cinema/foundation/schema/schema_version.h"

namespace kivo::cinema::foundation::contract {

enum class ContractKind {
  PersistentSchema,
  RuntimeSnapshot,
  EventEnvelope,
  DecisionRecord,
  GovernanceRule,
};

struct ContractEntry final {
  std::string name;
  std::string owner_module;
  ContractKind kind{ContractKind::PersistentSchema};
  schema::SchemaVersion schema_version{};
  std::vector<std::string> allowed_readers;
  std::vector<std::string> allowed_writers;
};

}  // namespace kivo::cinema::foundation::contract
