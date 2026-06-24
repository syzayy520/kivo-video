#include <cassert>

#include "kivo/cinema_engine/color_science_core/fake_color_output_service.hpp"
#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/video_render_core/fake_presenter_service.hpp"
#include "kivo/cinema_engine/windows_hdr_core/fake_windows_hdr_service.hpp"

int main() {
  const auto metadata = kivo::cinema_engine::make_contract_metadata("trace.fake.color-hdr-presenter", "session.fake.0001");

  kivo::cinema_engine::ComplianceGateResult compliance_gate;
  compliance_gate.metadata = metadata;
  compliance_gate.compliance_gate_id = "compliance.fake.color-hdr-presenter";
  compliance_gate.compliance_trace_id = "compliance.trace.fake.color-hdr-presenter";
  compliance_gate.claim_level = "dv_metadata_detected";
  compliance_gate.allowed_claims = {"Dolby Vision-aware", "DV metadata detected"};
  compliance_gate.forbidden_claim_reasons = {"official Dolby Vision output requires certification"};

  const auto color_matrix = kivo::cinema_engine::build_fake_color_output_matrix(
      metadata,
      compliance_gate,
      "DV_Profile8",
      "HDR10_PQ_BT2020");
  assert(color_matrix.metadata.schema_version == "1.1");
  assert(color_matrix.compliance_gate_ref == compliance_gate.compliance_gate_id);
  assert(color_matrix.claim_level == compliance_gate.claim_level);
  assert(color_matrix.output_path == "DV_aware_experimental_mapping");

  const auto hdr_output = kivo::cinema_engine::build_fake_windows_hdr_output_contract(metadata, color_matrix);
  assert(hdr_output.hdr_enabled);
  assert(hdr_output.color_output_matrix_ref == color_matrix.matrix_id);
  assert(hdr_output.target_display_class == color_matrix.target_display_class);

  const auto presenter = kivo::cinema_engine::build_fake_video_presenter_contract(metadata, hdr_output, color_matrix);
  assert(presenter.hdr_output_ref == hdr_output.windows_hdr_output_id);
  assert(presenter.color_output_matrix_ref == color_matrix.matrix_id);
  assert(presenter.frame_pacing_mode == "exact-24p");
  assert(presenter.expected_pacing_quality == "Reference");

  return 0;
}

