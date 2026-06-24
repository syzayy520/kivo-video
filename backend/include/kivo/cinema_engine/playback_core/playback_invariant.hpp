#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/playback_core/playback_state.hpp"

namespace kivo::cinema_engine {

struct PlaybackInvariantInput {
  PlaybackState state{PlaybackState::Idle};
  bool has_active_session{false};
  bool has_graph_binding{false};
  std::optional<std::uint64_t> seek_generation;
  std::optional<std::uint64_t> source_generation;
  std::optional<std::uint64_t> track_generation;
  std::optional<std::string> recovery_action_ref;
  bool emitted_event_after_closed{false};
};

struct InvariantViolation {
  std::string code;
  std::string message;
};

std::vector<InvariantViolation> check_playback_invariants(const PlaybackInvariantInput& input);

}  // namespace kivo::cinema_engine

