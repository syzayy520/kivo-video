#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct VideoPresenterContract {
  ContractMetadata metadata;
  std::string presenter_contract_id;
  std::optional<std::string> session_id;
  std::string presenter_backend;
  std::string vsync_source;
  double refresh_rate{0.0};
  double content_frame_rate{0.0};
  std::string frame_pacing_mode;
  std::string clock_master;
  std::string late_frame_policy;
  std::string drop_duplicate_policy;
  std::string jitter_budget;
  std::string zero_copy_state;
  std::optional<std::string> copy_back_reason;
  std::optional<std::string> hdr_output_ref;
  std::optional<std::string> color_output_matrix_ref;
  std::string expected_pacing_quality;
  std::optional<std::string> fallback_reason;
};

}  // namespace kivo::cinema_engine

