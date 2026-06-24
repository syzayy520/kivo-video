#include "kivo/cinema_engine/color_science_core/fake_color_output_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

ColorOutputMatrix build_fake_color_output_matrix(const ContractMetadata& metadata,
                                                 const ComplianceGateResult& compliance_gate,
                                                 std::string input_signal,
                                                 std::string target_display_class) {
  ColorOutputMatrix matrix;
  matrix.metadata = metadata;
  matrix.matrix_id = "color.matrix.fake.service.0001";
  matrix.session_id = metadata.session_id;
  matrix.input_signal = std::move(input_signal);
  matrix.preserved_metadata = "mastering_display,max_cll,max_fall,rpu_presence";
  matrix.target_display_class = std::move(target_display_class);
  matrix.output_path = "DV_aware_experimental_mapping";
  matrix.output_transfer = "PQ";
  matrix.output_color_space = "BT2020";
  matrix.tone_mapping_mode = "metadata-aware";
  matrix.gamut_mapping_mode = "preserve-bt2020";
  matrix.hdr_metadata_policy = "set-hdr10-static-metadata";
  matrix.dv_fallback_tier = "dv-aware-compatibility";
  matrix.claim_level = compliance_gate.claim_level;
  matrix.quality_tier = compliance_gate.fail_closed ? "Rejected" : "High";
  matrix.compliance_gate_ref = compliance_gate.compliance_gate_id;
  return matrix;
}

}  // namespace kivo::cinema_engine

