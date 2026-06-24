#include "kivo/cinema_engine/video_render_core/fake_presenter_service.hpp"

namespace kivo::cinema_engine {

VideoPresenterContract build_fake_video_presenter_contract(const ContractMetadata& metadata,
                                                           const WindowsHdrOutputContract& hdr_output,
                                                           const ColorOutputMatrix& color_output_matrix) {
  VideoPresenterContract contract;
  contract.metadata = metadata;
  contract.presenter_contract_id = "presenter.fake.service.0001";
  contract.session_id = metadata.session_id;
  contract.presenter_backend = "fake-d3d-adapter";
  contract.vsync_source = "display-vblank";
  contract.refresh_rate = 23.976;
  contract.content_frame_rate = 23.976;
  contract.frame_pacing_mode = "exact-24p";
  contract.clock_master = "audio_clock";
  contract.late_frame_policy = "drop-if-over-budget";
  contract.drop_duplicate_policy = "preserve-cadence";
  contract.jitter_budget = "2ms";
  contract.zero_copy_state = "available";
  contract.hdr_output_ref = hdr_output.windows_hdr_output_id;
  contract.color_output_matrix_ref = color_output_matrix.matrix_id;
  contract.expected_pacing_quality = hdr_output.hdr_enabled ? "Reference" : "Compatible";
  if (!hdr_output.hdr_enabled) {
    contract.fallback_reason = "HDR output unavailable";
  }
  return contract;
}

}  // namespace kivo::cinema_engine

