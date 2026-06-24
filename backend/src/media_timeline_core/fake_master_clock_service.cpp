#include "kivo/cinema_engine/media_timeline_core/fake_master_clock_service.hpp"

namespace kivo::cinema_engine {

MasterClock build_audio_master_clock() {
  MasterClock clock;
  clock.metadata.schema_version = "1.1";
  clock.clock_id = "clock.audio-master.001";
  clock.clock_source = "audio";
  clock.current_time_us = 0;
  clock.drift_ms = 0.0;
  clock.is_running = true;
  clock.is_synced = true;
  return clock;
}

MasterClock build_stopped_clock() {
  MasterClock clock;
  clock.metadata.schema_version = "1.1";
  clock.clock_id = "clock.stopped.001";
  clock.clock_source = "audio";
  clock.current_time_us = 0;
  clock.drift_ms = 0.0;
  clock.is_running = false;
  clock.is_synced = false;
  return clock;
}

AvSyncPolicy build_default_av_sync_policy() {
  AvSyncPolicy policy;
  policy.metadata.schema_version = "1.1";
  policy.policy_id = "av-sync.default.local";
  policy.sync_mode = "audio_master";
  policy.max_drift_ms = 40.0;
  policy.correction_threshold_ms = 20.0;
  policy.correction_strategy = "skip_frame";
  policy.allow_video_drop = true;
  policy.allow_audio_resample = false;
  return policy;
}

AvSyncPolicy build_av_sync_policy_with_drift(double max_drift_ms) {
  AvSyncPolicy policy;
  policy.metadata.schema_version = "1.1";
  policy.policy_id = "av-sync.custom.drift";
  policy.sync_mode = "audio_master";
  policy.max_drift_ms = max_drift_ms;
  policy.correction_threshold_ms = max_drift_ms / 2.0;
  policy.correction_strategy = "skip_frame";
  policy.allow_video_drop = true;
  policy.allow_audio_resample = false;
  return policy;
}

}  // namespace kivo::cinema_engine
