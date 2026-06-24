#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/playback_core/playback_state.hpp"

namespace kivo::cinema_engine {

// Valid state transition pair.
struct StateTransitionRule {
  PlaybackState from;
  PlaybackState to;
  std::string trigger;
  std::optional<std::string> condition;
};

// Result of a state transition attempt.
enum class TransitionDecision {
  Accepted,
  RejectedInvalidTransition,
  RejectedInvariantViolation,
  RejectedTerminalState
};

struct StateTransitionResult {
  TransitionDecision decision;
  PlaybackState previous_state;
  PlaybackState new_state;
  std::string trigger;
  std::optional<std::string> rejection_reason;
};

// Playback session state machine contract.
struct PlaybackSessionStateMachineContract {
  ContractMetadata metadata;
  PlaybackState current_state{PlaybackState::Idle};
  std::vector<StateTransitionRule> transition_rules;
  std::vector<StateTransitionResult> transition_log;
};

// Check if a transition is valid.
bool is_valid_transition(PlaybackState from, PlaybackState to);

// Get all valid target states from a given state.
std::vector<PlaybackState> valid_targets_from(PlaybackState from);

// Attempt a state transition and return the result.
StateTransitionResult attempt_transition(
    PlaybackState current,
    PlaybackState target,
    const std::string& trigger);

// Get human-readable state name.
const char* playback_state_name(PlaybackState state);

}  // namespace kivo::cinema_engine
