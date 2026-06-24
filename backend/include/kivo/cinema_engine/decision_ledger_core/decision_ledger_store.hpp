#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/decision_ledger_core/decision_ledger_entry.hpp"

namespace kivo::cinema_engine {

class DecisionLedgerStore {
 public:
  bool append(DecisionLedgerEntry entry);
  std::vector<DecisionLedgerEntry> entries_for_session(const std::string& session_id) const;
  std::vector<DecisionLedgerEntry> export_all() const;
  std::optional<DecisionLedgerEntry> find_entry(const std::string& entry_id) const;
  std::size_t size() const;

 private:
  std::vector<DecisionLedgerEntry> entries_;
};

}  // namespace kivo::cinema_engine
