#include "kivo/cinema_engine/playback_core/fake_playback_state_machine.hpp"

#include <utility>

namespace kivo::cinema_engine {

bool FakePlaybackStateMachine::open(std::string session_id) {
  if (state_ != PlaybackState::Idle || session_id.empty()) {
    return false;
  }

  session_id_ = std::move(session_id);
  state_ = PlaybackState::Ready;
  return true;
}

bool FakePlaybackStateMachine::bind_graph(std::string graph_id) {
  if (!session_id_.has_value() || graph_id.empty() || state_ == PlaybackState::Closed) {
    return false;
  }

  graph_id_ = std::move(graph_id);
  return true;
}

bool FakePlaybackStateMachine::play() {
  if (!session_id_.has_value() || !graph_id_.has_value() || state_ == PlaybackState::Closed) {
    return false;
  }

  state_ = PlaybackState::Playing;
  return invariant_violations().empty();
}

bool FakePlaybackStateMachine::pause() {
  if (state_ != PlaybackState::Playing) {
    return false;
  }

  state_ = PlaybackState::Paused;
  return true;
}

bool FakePlaybackStateMachine::seek(std::uint64_t seek_generation) {
  if (!session_id_.has_value() || state_ == PlaybackState::Closed || seek_generation == 0) {
    return false;
  }

  seek_generation_ = seek_generation;
  state_ = PlaybackState::Seeking;
  return invariant_violations().empty();
}

bool FakePlaybackStateMachine::complete_seek() {
  if (state_ != PlaybackState::Seeking || !seek_generation_.has_value()) {
    return false;
  }

  seek_generation_.reset();
  state_ = graph_id_.has_value() ? PlaybackState::Playing : PlaybackState::Ready;
  return invariant_violations().empty();
}

bool FakePlaybackStateMachine::recover(std::string recovery_action_ref) {
  if (!session_id_.has_value() || recovery_action_ref.empty() || state_ == PlaybackState::Closed) {
    return false;
  }

  recovery_action_ref_ = std::move(recovery_action_ref);
  state_ = PlaybackState::Recovering;
  return invariant_violations().empty();
}

bool FakePlaybackStateMachine::close() {
  if (state_ == PlaybackState::Closed) {
    emitted_event_after_closed_ = true;
    return false;
  }

  state_ = PlaybackState::Closed;
  return invariant_violations().empty();
}

PlaybackState FakePlaybackStateMachine::state() const {
  return state_;
}

PlaybackInvariantInput FakePlaybackStateMachine::invariant_input() const {
  PlaybackInvariantInput input;
  input.state = state_;
  input.has_active_session = session_id_.has_value();
  input.has_graph_binding = graph_id_.has_value();
  input.seek_generation = seek_generation_;
  input.recovery_action_ref = recovery_action_ref_;
  input.emitted_event_after_closed = emitted_event_after_closed_;
  return input;
}

std::vector<InvariantViolation> FakePlaybackStateMachine::invariant_violations() const {
  return check_playback_invariants(invariant_input());
}

}  // namespace kivo::cinema_engine

