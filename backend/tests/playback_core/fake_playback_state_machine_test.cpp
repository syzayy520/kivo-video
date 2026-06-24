#include <cassert>

#include "kivo/cinema_engine/playback_core/fake_playback_state_machine.hpp"

int main() {
  kivo::cinema_engine::FakePlaybackStateMachine machine;

  assert(!machine.play());
  assert(machine.open("session.fake.0001"));
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Ready);
  assert(!machine.play());
  assert(machine.bind_graph("graph.fake.0001"));
  assert(machine.play());
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Playing);
  assert(machine.pause());
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Paused);
  assert(machine.seek(1));
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Seeking);
  assert(machine.complete_seek());
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Playing);
  assert(machine.recover("recovery.fake.refresh-link"));
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Recovering);
  assert(machine.close());
  assert(machine.state() == kivo::cinema_engine::PlaybackState::Closed);
  assert(!machine.close());
  assert(!machine.invariant_violations().empty());

  return 0;
}

