#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct MediaExperienceSnapshot {
  ContractMetadata metadata;
  std::string media_experience_snapshot_id;
  std::string media_id;
  std::optional<std::string> version_id;
  std::string title;
  std::optional<std::string> original_title;
  std::optional<std::string> sort_title;
  std::optional<int> year;
  std::optional<std::string> release_date;
  std::optional<std::string> poster_art_ref;
  std::optional<std::string> backdrop_art_ref;
  std::optional<std::string> logo_art_ref;
  std::optional<std::string> collection_ref;
  std::optional<std::string> season_episode_ref;
  double playback_progress{0.0};
  std::string watched_state;
  std::optional<std::string> resume_point;
  std::string version_summary;
  std::string source_availability_summary;
  std::string subtitle_availability_summary;
  std::string audio_quality_summary;
  std::string video_quality_summary;
  std::optional<std::string> hdr_badge_claim_ref;
  std::vector<std::string> user_visible_quality_badges;
  std::string metadata_provenance;
  std::optional<std::string> manual_override_state;
};

}  // namespace kivo::cinema_engine

