#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/playback_core/playback_invariant.hpp"
#include "kivo/cinema_engine/playback_core/playback_state.hpp"

namespace kivo::cinema_engine {

class FakePlaybackStateMachine {
 public:
  bool open(std::string session_id);
  bool bind_graph(std::string graph_id);
  bool play();
  bool pause();
  bool seek(std::uint64_t seek_generation);
  bool complete_seek();
  bool recover(std::string recovery_action_ref);
  bool close();

  PlaybackState state() const;
  PlaybackInvariantInput invariant_input() const;
  std::vector<InvariantViolation> invariant_violations() const;

 private:
  PlaybackState state_{PlaybackState::Idle};
  std::optional<std::string> session_id_;
  std::optional<std::string> graph_id_;
  std::optional<std::uint64_t> seek_generation_;
  std::optional<std::string> recovery_action_ref_;
  bool emitted_event_after_closed_{false};
};

}  // namespace kivo::cinema_engine

