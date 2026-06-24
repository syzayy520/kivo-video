#include "kivo/cinema_engine/decision_ledger_core/replay_manifest.hpp"

#include <cstdint>

namespace kivo::cinema_engine {

ReplayManifest build_replay_manifest(const PlaybackSessionAggregate& session,
                                     const std::vector<DecisionLedgerEntry>& entries) {
  ReplayManifest manifest;
  manifest.metadata = make_contract_metadata(session.trace_root_id, session.session_id);
  manifest.replay_manifest_id = "replay." + session.session_id;
  manifest.session_id = session.session_id;
  manifest.decision_ledger_id = session.decision_ledger_id;

  std::uint64_t expected_sequence = 1;
  for (const auto& entry : entries) {
    if (entry.session_id != session.session_id) {
      manifest.replay_blocker = "entry session mismatch";
      return manifest;
    }
    if (entry.sequence_number != expected_sequence) {
      manifest.replay_blocker = "non-contiguous sequence";
      return manifest;
    }
    if (!entry.generation.has_value()) {
      manifest.replay_blocker = "missing generation";
      return manifest;
    }
    manifest.ordered_entry_ids.push_back(entry.entry_id);
    ++expected_sequence;
  }

  manifest.replayable = !manifest.ordered_entry_ids.empty();
  if (!manifest.replayable) {
    manifest.replay_blocker = "empty ledger";
  }
  return manifest;
}

}  // namespace kivo::cinema_engine
