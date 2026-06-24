#pragma once

#include "kivo/cinema_engine/media_timeline_core/master_clock.hpp"
#include "kivo/cinema_engine/media_timeline_core/av_sync_policy.hpp"

namespace kivo::cinema_engine {

// Build an audio-master clock for local playback.
MasterClock build_audio_master_clock();

// Build a stopped clock.
MasterClock build_stopped_clock();

// Build default AV sync policy for local playback.
AvSyncPolicy build_default_av_sync_policy();

// Build AV sync policy with custom drift tolerance.
AvSyncPolicy build_av_sync_policy_with_drift(double max_drift_ms);

}  // namespace kivo::cinema_engine
