#include "kivo/cinema_engine/playback_core/playback_session_state_machine.hpp"

namespace kivo::cinema_engine {

namespace {

// Transition table: from -> to pairs that are valid.
struct TransitionEntry {
  PlaybackState from;
  PlaybackState to;
};

constexpr TransitionEntry kValidTransitions[] = {
  // Startup sequence
  {PlaybackState::Idle, PlaybackState::Opening},
  {PlaybackState::Opening, PlaybackState::CreatingSession},
  {PlaybackState::CreatingSession, PlaybackState::ResolvingSource},
  {PlaybackState::ResolvingSource, PlaybackState::SelectingVersion},
  {PlaybackState::SelectingVersion, PlaybackState::PlanningProbe},
  {PlaybackState::PlanningProbe, PlaybackState::JoiningCapability},
  {PlaybackState::JoiningCapability, PlaybackState::PlanningPlayback},
  {PlaybackState::PlanningPlayback, PlaybackState::CheckingCompliance},
  {PlaybackState::CheckingCompliance, PlaybackState::CheckingFeatureGate},
  {PlaybackState::CheckingFeatureGate, PlaybackState::CheckingResourceBudget},
  {PlaybackState::CheckingResourceBudget, PlaybackState::PreparingGraph},
  {PlaybackState::PreparingGraph, PlaybackState::PreparingCache},
  {PlaybackState::PreparingCache, PlaybackState::ProbingMedia},
  {PlaybackState::ProbingMedia, PlaybackState::SelectingDecoder},
  {PlaybackState::SelectingDecoder, PlaybackState::PreparingRender},
  {PlaybackState::PreparingRender, PlaybackState::Ready},

  // Playback controls
  {PlaybackState::Ready, PlaybackState::Playing},
  {PlaybackState::Playing, PlaybackState::Paused},
  {PlaybackState::Paused, PlaybackState::Playing},
  {PlaybackState::Playing, PlaybackState::Seeking},
  {PlaybackState::Paused, PlaybackState::Seeking},
  {PlaybackState::Seeking, PlaybackState::Playing},
  {PlaybackState::Seeking, PlaybackState::Paused},

  // Buffering
  {PlaybackState::Playing, PlaybackState::Buffering},
  {PlaybackState::Buffering, PlaybackState::Playing},

  // Source/track switching
  {PlaybackState::Playing, PlaybackState::SwitchingSource},
  {PlaybackState::SwitchingSource, PlaybackState::Playing},
  {PlaybackState::Playing, PlaybackState::SwitchingTrack},
  {PlaybackState::SwitchingTrack, PlaybackState::Playing},

  // Reevaluation
  {PlaybackState::Playing, PlaybackState::ReevaluatingPlan},
  {PlaybackState::Paused, PlaybackState::ReevaluatingPlan},
  {PlaybackState::ReevaluatingPlan, PlaybackState::Playing},

  // Recovery
  {PlaybackState::Playing, PlaybackState::Recovering},
  {PlaybackState::Paused, PlaybackState::Recovering},
  {PlaybackState::Buffering, PlaybackState::Recovering},
  {PlaybackState::Seeking, PlaybackState::Recovering},
  {PlaybackState::Recovering, PlaybackState::Playing},
  {PlaybackState::Recovering, PlaybackState::Ready},

  // Drain/End
  {PlaybackState::Playing, PlaybackState::Draining},
  {PlaybackState::Draining, PlaybackState::Ended},

  // Failure
  {PlaybackState::Playing, PlaybackState::Failed},
  {PlaybackState::Paused, PlaybackState::Failed},
  {PlaybackState::Seeking, PlaybackState::Failed},
  {PlaybackState::Buffering, PlaybackState::Failed},
  {PlaybackState::Recovering, PlaybackState::Failed},
  {PlaybackState::Failed, PlaybackState::Closed},

  // Close from any non-terminal state
  {PlaybackState::Idle, PlaybackState::Closed},
  {PlaybackState::Opening, PlaybackState::Closed},
  {PlaybackState::CreatingSession, PlaybackState::Closed},
  {PlaybackState::ResolvingSource, PlaybackState::Closed},
  {PlaybackState::SelectingVersion, PlaybackState::Closed},
  {PlaybackState::PlanningProbe, PlaybackState::Closed},
  {PlaybackState::JoiningCapability, PlaybackState::Closed},
  {PlaybackState::PlanningPlayback, PlaybackState::Closed},
  {PlaybackState::CheckingCompliance, PlaybackState::Closed},
  {PlaybackState::CheckingFeatureGate, PlaybackState::Closed},
  {PlaybackState::CheckingResourceBudget, PlaybackState::Closed},
  {PlaybackState::PreparingGraph, PlaybackState::Closed},
  {PlaybackState::PreparingCache, PlaybackState::Closed},
  {PlaybackState::ProbingMedia, PlaybackState::Closed},
  {PlaybackState::SelectingDecoder, PlaybackState::Closed},
  {PlaybackState::PreparingRender, PlaybackState::Closed},
  {PlaybackState::Ready, PlaybackState::Closed},
  {PlaybackState::Playing, PlaybackState::Closed},
  {PlaybackState::Paused, PlaybackState::Closed},
  {PlaybackState::Seeking, PlaybackState::Closed},
  {PlaybackState::Buffering, PlaybackState::Closed},
  {PlaybackState::SwitchingSource, PlaybackState::Closed},
  {PlaybackState::SwitchingTrack, PlaybackState::Closed},
  {PlaybackState::ReevaluatingPlan, PlaybackState::Closed},
  {PlaybackState::Recovering, PlaybackState::Closed},
  {PlaybackState::Draining, PlaybackState::Closed},
  {PlaybackState::Ended, PlaybackState::Closed},
};

constexpr std::size_t kValidTransitionCount = sizeof(kValidTransitions) / sizeof(kValidTransitions[0]);

}  // namespace

bool is_valid_transition(PlaybackState from, PlaybackState to) {
  for (std::size_t i = 0; i < kValidTransitionCount; ++i) {
    if (kValidTransitions[i].from == from && kValidTransitions[i].to == to) {
      return true;
    }
  }
  return false;
}

std::vector<PlaybackState> valid_targets_from(PlaybackState from) {
  std::vector<PlaybackState> targets;
  for (std::size_t i = 0; i < kValidTransitionCount; ++i) {
    if (kValidTransitions[i].from == from) {
      targets.push_back(kValidTransitions[i].to);
    }
  }
  return targets;
}

StateTransitionResult attempt_transition(
    PlaybackState current,
    PlaybackState target,
    const std::string& trigger) {

  StateTransitionResult result;
  result.previous_state = current;
  result.new_state = current;
  result.trigger = trigger;

  if (current == PlaybackState::Closed) {
    result.decision = TransitionDecision::RejectedTerminalState;
    result.rejection_reason = "Cannot transition from Closed state";
    return result;
  }

  if (current == PlaybackState::Failed && target != PlaybackState::Closed) {
    result.decision = TransitionDecision::RejectedTerminalState;
    result.rejection_reason = "Failed state can only transition to Closed";
    return result;
  }

  if (!is_valid_transition(current, target)) {
    result.decision = TransitionDecision::RejectedInvalidTransition;
    result.rejection_reason = std::string("Invalid transition from ") +
        playback_state_name(current) + " to " + playback_state_name(target);
    return result;
  }

  result.decision = TransitionDecision::Accepted;
  result.new_state = target;
  return result;
}

const char* playback_state_name(PlaybackState state) {
  switch (state) {
    case PlaybackState::Idle: return "Idle";
    case PlaybackState::Opening: return "Opening";
    case PlaybackState::CreatingSession: return "CreatingSession";
    case PlaybackState::ResolvingSource: return "ResolvingSource";
    case PlaybackState::SelectingVersion: return "SelectingVersion";
    case PlaybackState::PlanningProbe: return "PlanningProbe";
    case PlaybackState::JoiningCapability: return "JoiningCapability";
    case PlaybackState::PlanningPlayback: return "PlanningPlayback";
    case PlaybackState::CheckingCompliance: return "CheckingCompliance";
    case PlaybackState::CheckingFeatureGate: return "CheckingFeatureGate";
    case PlaybackState::CheckingResourceBudget: return "CheckingResourceBudget";
    case PlaybackState::PreparingGraph: return "PreparingGraph";
    case PlaybackState::PreparingCache: return "PreparingCache";
    case PlaybackState::ProbingMedia: return "ProbingMedia";
    case PlaybackState::SelectingDecoder: return "SelectingDecoder";
    case PlaybackState::PreparingRender: return "PreparingRender";
    case PlaybackState::Ready: return "Ready";
    case PlaybackState::Playing: return "Playing";
    case PlaybackState::Paused: return "Paused";
    case PlaybackState::Seeking: return "Seeking";
    case PlaybackState::Buffering: return "Buffering";
    case PlaybackState::SwitchingSource: return "SwitchingSource";
    case PlaybackState::SwitchingTrack: return "SwitchingTrack";
    case PlaybackState::ReevaluatingPlan: return "ReevaluatingPlan";
    case PlaybackState::Recovering: return "Recovering";
    case PlaybackState::Draining: return "Draining";
    case PlaybackState::Ended: return "Ended";
    case PlaybackState::Failed: return "Failed";
    case PlaybackState::Closed: return "Closed";
  }
  return "Unknown";
}

}  // namespace kivo::cinema_engine
