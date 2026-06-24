#include "kivo/cinema_engine/playback_core/playback_invariant.hpp"

namespace kivo::cinema_engine {

std::vector<InvariantViolation> check_playback_invariants(const PlaybackInvariantInput& input) {
  std::vector<InvariantViolation> violations;

  if (input.state == PlaybackState::Playing && !input.has_active_session) {
    violations.push_back({"PLAYING_REQUIRES_ACTIVE_SESSION", "Playing must have an active session."});
  }

  if (input.state == PlaybackState::Playing && !input.has_graph_binding) {
    violations.push_back({"PLAYING_REQUIRES_GRAPH_BINDING", "Playing must have an active graph binding."});
  }

  if (input.state == PlaybackState::Seeking && !input.seek_generation.has_value()) {
    violations.push_back({"SEEKING_REQUIRES_SEEK_GENERATION", "Seeking must have a seek generation."});
  }

  if (input.state == PlaybackState::SwitchingSource && !input.source_generation.has_value()) {
    violations.push_back({"SWITCHING_SOURCE_REQUIRES_SOURCE_GENERATION", "SwitchingSource must have a source generation."});
  }

  if (input.state == PlaybackState::SwitchingTrack && !input.track_generation.has_value()) {
    violations.push_back({"SWITCHING_TRACK_REQUIRES_TRACK_GENERATION", "SwitchingTrack must have a track generation."});
  }

  if (input.state == PlaybackState::Recovering && !input.recovery_action_ref.has_value()) {
    violations.push_back({"RECOVERING_REQUIRES_RECOVERY_ACTION", "Recovering must have a recovery action ref."});
  }

  if (input.state == PlaybackState::Closed && input.emitted_event_after_closed) {
    violations.push_back({"CLOSED_FORBIDS_LATE_EVENT", "Closed sessions must not emit playback events."});
  }

  return violations;
}

}  // namespace kivo::cinema_engine

