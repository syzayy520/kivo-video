#include "kivo/cinema_engine/windows_hdr_core/fake_windows_hdr_service.hpp"

namespace kivo::cinema_engine {

WindowsHdrOutputContract build_fake_windows_hdr_output_contract(const ContractMetadata& metadata,
                                                                const ColorOutputMatrix& color_output_matrix) {
  WindowsHdrOutputContract contract;
  contract.metadata = metadata;
  contract.windows_hdr_output_id = "winhdr.fake.service.0001";
  contract.session_id = metadata.session_id;
  contract.display_id = "display.fake.primary";
  contract.monitor_luid = "monitor.fake.luid";
  contract.gpu_adapter_id = "gpu.fake.adapter";
  contract.advanced_color_available = true;
  contract.hdr_enabled = color_output_matrix.target_display_class == "HDR10_PQ_BT2020";
  contract.color_space = "DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
  contract.swapchain_format = "R10G10B10A2_UNORM";
  contract.output_bit_depth = 10;
  contract.fullscreen_state = "borderless";
  contract.window_occlusion_state = "visible";
  contract.multi_monitor_state = "single-active-hdr";
  contract.os_tone_mapping_mode = "advanced-color";
  contract.hdr_metadata_set_result = "fake-success";
  contract.display_luminance_caps = "min-0.005,max-1000";
  contract.target_display_class = color_output_matrix.target_display_class;
  contract.color_output_matrix_ref = color_output_matrix.matrix_id;
  contract.output_confidence = contract.hdr_enabled ? 0.9 : 0.5;
  if (!contract.hdr_enabled) {
    contract.fallback_reason = "target display is not HDR";
  }
  return contract;
}

}  // namespace kivo::cinema_engine

