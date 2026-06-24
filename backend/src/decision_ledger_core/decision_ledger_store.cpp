#include "kivo/cinema_engine/decision_ledger_core/decision_ledger_store.hpp"

#include <algorithm>
#include <utility>

namespace kivo::cinema_engine {

bool DecisionLedgerStore::append(DecisionLedgerEntry entry) {
  if (entry.entry_id.empty() || entry.session_id.empty() || entry.sequence_number == 0 || !entry.generation.has_value()) {
    return false;
  }

  const auto duplicate = std::find_if(entries_.begin(), entries_.end(), [&entry](const DecisionLedgerEntry& existing) {
    return existing.entry_id == entry.entry_id;
  });
  if (duplicate != entries_.end()) {
    return false;
  }

  const auto same_session_latest = std::find_if(entries_.rbegin(), entries_.rend(), [&entry](const DecisionLedgerEntry& existing) {
    return existing.session_id == entry.session_id;
  });
  if (same_session_latest != entries_.rend() && entry.sequence_number <= same_session_latest->sequence_number) {
    return false;
  }

  entries_.push_back(std::move(entry));
  return true;
}

std::vector<DecisionLedgerEntry> DecisionLedgerStore::entries_for_session(const std::string& session_id) const {
  std::vector<DecisionLedgerEntry> session_entries;
  for (const auto& entry : entries_) {
    if (entry.session_id == session_id) {
      session_entries.push_back(entry);
    }
  }
  return session_entries;
}

std::vector<DecisionLedgerEntry> DecisionLedgerStore::export_all() const {
  return entries_;
}

std::optional<DecisionLedgerEntry> DecisionLedgerStore::find_entry(const std::string& entry_id) const {
  const auto found = std::find_if(entries_.begin(), entries_.end(), [&entry_id](const DecisionLedgerEntry& entry) {
    return entry.entry_id == entry_id;
  });
  if (found == entries_.end()) {
    return std::nullopt;
  }
  return *found;
}

std::size_t DecisionLedgerStore::size() const {
  return entries_.size();
}

}  // namespace kivo::cinema_engine

