#include <cassert>
#include <optional>

#include "kivo/cinema_engine/decision_ledger_core/decision_ledger_store.hpp"
#include "kivo/cinema_engine/fake_backend/fake_session_scenario.hpp"
#include "kivo/cinema_engine/session_core/session_store.hpp"

int main() {
  const auto scenario = kivo::cinema_engine::build_fake_direct_play_session();

  kivo::cinema_engine::SessionStore session_store;
  auto open_session = scenario.session;
  open_session.closed_at = std::nullopt;
  open_session.close_reason = std::nullopt;
  assert(session_store.create(open_session));
  assert(!session_store.create(open_session));
  const auto found_session = session_store.find(scenario.session.session_id);
  assert(found_session.has_value());
  assert(found_session->session_id == scenario.session.session_id);
  assert(session_store.close(scenario.session.session_id, "2026-06-24T00:00:10Z", "fake-session-complete"));
  assert(session_store.export_all().size() == 1);

  kivo::cinema_engine::DecisionLedgerStore ledger_store;
  for (const auto& entry : scenario.decision_ledger) {
    assert(ledger_store.append(entry));
  }
  assert(ledger_store.size() == scenario.decision_ledger.size());
  assert(ledger_store.entries_for_session(scenario.session.session_id).size() == scenario.decision_ledger.size());
  assert(ledger_store.find_entry("ledger.fake.entry.1").has_value());
  assert(ledger_store.export_all().size() == scenario.decision_ledger.size());

  auto broken_entry = scenario.decision_ledger.front();
  broken_entry.entry_id = "ledger.fake.entry.broken";
  broken_entry.sequence_number = 1;
  assert(!ledger_store.append(broken_entry));

  return 0;
}
