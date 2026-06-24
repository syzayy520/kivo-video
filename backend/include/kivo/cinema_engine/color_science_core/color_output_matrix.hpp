#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct ColorOutputMatrix {
  ContractMetadata metadata;
  std::string matrix_id;
  std::optional<std::string> session_id;
  std::string input_signal;
  std::string preserved_metadata;
  std::string target_display_class;
  std::string output_path;
  std::string output_transfer;
  std::string output_color_space;
  std::string tone_mapping_mode;
  std::string gamut_mapping_mode;
  std::string hdr_metadata_policy;
  std::string dv_fallback_tier;
  std::string claim_level;
  std::string quality_tier;
  std::string compliance_gate_ref;
};

}  // namespace kivo::cinema_engine

