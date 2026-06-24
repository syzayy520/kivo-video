#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/decision_ledger_core/decision_ledger_entry.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/session_core/playback_session_aggregate.hpp"

namespace kivo::cinema_engine {

struct ReplayManifest {
  ContractMetadata metadata;
  std::string replay_manifest_id;
  std::string session_id;
  std::string decision_ledger_id;
  std::vector<std::string> ordered_entry_ids;
  bool replayable{false};
  std::string replay_blocker;
};

ReplayManifest build_replay_manifest(const PlaybackSessionAggregate& session,
                                     const std::vector<DecisionLedgerEntry>& entries);

}  // namespace kivo::cinema_engine

