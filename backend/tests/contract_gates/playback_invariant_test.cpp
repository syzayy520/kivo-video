#include <cassert>

#include "kivo/cinema_engine/playback_core/playback_invariant.hpp"

int diagnostic_redaction_gate_test();

int main() {
  diagnostic_redaction_gate_test();

  kivo::cinema_engine::PlaybackInvariantInput broken_playing;
  broken_playing.state = kivo::cinema_engine::PlaybackState::Playing;
  const auto broken_violations = kivo::cinema_engine::check_playback_invariants(broken_playing);
  assert(broken_violations.size() == 2);

  kivo::cinema_engine::PlaybackInvariantInput valid_playing;
  valid_playing.state = kivo::cinema_engine::PlaybackState::Playing;
  valid_playing.has_active_session = true;
  valid_playing.has_graph_binding = true;
  const auto valid_violations = kivo::cinema_engine::check_playback_invariants(valid_playing);
  assert(valid_violations.empty());

  kivo::cinema_engine::PlaybackInvariantInput broken_closed;
  broken_closed.state = kivo::cinema_engine::PlaybackState::Closed;
  broken_closed.emitted_event_after_closed = true;
  const auto closed_violations = kivo::cinema_engine::check_playback_invariants(broken_closed);
  assert(closed_violations.size() == 1);

  return 0;
}

