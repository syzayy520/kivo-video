#pragma once

#include <string>
#include <vector>

namespace kivo::cinema_engine {

enum class ContractPersistence {
  MemoryOnly,
  Persistent,
  AppendOnly,
  Snapshot,
  RingBuffer,
  ExportOnly
};

struct ContractRegistration {
  std::string name;
  std::string owner_module;
  std::string schema_version;
  ContractPersistence persistence;
  std::vector<std::string> allowed_writers;
  std::vector<std::string> allowed_readers;
};

const std::vector<ContractRegistration>& core_contract_registry();
bool registry_contains_contract(const std::string& name);

}  // namespace kivo::cinema_engine

