#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct PlaybackInspectorSnapshot {
  ContractMetadata metadata;
  std::string inspector_snapshot_id;
  std::optional<std::string> session_id;
  std::string user_summary;
  std::string source_summary;
  std::string network_summary;
  std::string cache_summary;
  std::string video_summary;
  std::string hdr_dv_summary;
  std::string audio_summary;
  std::string subtitle_summary;
  std::string fallback_summary;
  std::string quality_impact_summary;
  std::vector<std::string> actions_available;
  std::optional<std::string> developer_trace_ref;
  std::string privacy_level;
  std::string redaction_state;
};

}  // namespace kivo::cinema_engine

