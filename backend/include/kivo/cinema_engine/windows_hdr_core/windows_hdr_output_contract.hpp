#pragma once

#include <optional>
#include <string>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct WindowsHdrOutputContract {
  ContractMetadata metadata;
  std::string windows_hdr_output_id;
  std::optional<std::string> session_id;
  std::string display_id;
  std::optional<std::string> monitor_luid;
  std::optional<std::string> gpu_adapter_id;
  bool advanced_color_available{false};
  bool hdr_enabled{false};
  std::string color_space;
  std::string swapchain_format;
  int output_bit_depth{8};
  std::string fullscreen_state;
  std::string window_occlusion_state;
  std::string multi_monitor_state;
  std::string os_tone_mapping_mode;
  std::string hdr_metadata_set_result;
  std::string display_luminance_caps;
  std::string target_display_class;
  std::string color_output_matrix_ref;
  double output_confidence{0.0};
  std::optional<std::string> fallback_reason;
};

}  // namespace kivo::cinema_engine

