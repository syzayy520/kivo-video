#pragma once

namespace kivo::cinema_engine {

enum class PlaybackState {
  Idle,
  Opening,
  CreatingSession,
  ResolvingSource,
  SelectingVersion,
  PlanningProbe,
  JoiningCapability,
  PlanningPlayback,
  CheckingCompliance,
  CheckingFeatureGate,
  CheckingResourceBudget,
  PreparingGraph,
  PreparingCache,
  ProbingMedia,
  SelectingDecoder,
  PreparingRender,
  Ready,
  Playing,
  Paused,
  Seeking,
  Buffering,
  SwitchingSource,
  SwitchingTrack,
  ReevaluatingPlan,
  Recovering,
  Draining,
  Ended,
  Failed,
  Closed
};

}  // namespace kivo::cinema_engine

