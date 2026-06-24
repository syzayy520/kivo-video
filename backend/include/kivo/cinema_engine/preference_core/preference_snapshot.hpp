#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

// Subtitle preferences.
struct SubtitlePreference {
  std::optional<std::string> language;
  std::string style{"default"};
  std::string position{"bottom"};
  bool enabled{false};
};

// Audio preferences.
struct AudioPreference {
  std::optional<std::string> language;
  std::string channel_layout{"stereo"};
  double volume{1.0};
  bool normalize{false};
};

// Video quality preferences.
struct VideoPreference {
  std::optional<std::string> max_resolution;
  std::string hdr_preference{"auto"};
  bool deinterlace{true};
};

// Playback behavior preferences.
struct PlaybackPreference {
  bool auto_play{true};
  bool auto_next{true};
  bool resume_from_last{true};
};

// Complete user preference snapshot.
struct PreferenceSnapshot {
  ContractMetadata metadata;
  SubtitlePreference subtitle;
  AudioPreference audio;
  VideoPreference video;
  PlaybackPreference playback;
};

}  // namespace kivo::cinema_engine
