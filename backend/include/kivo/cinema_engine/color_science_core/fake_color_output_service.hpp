#pragma once

#include <string>

#include "kivo/cinema_engine/color_science_core/color_output_matrix.hpp"
#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

ColorOutputMatrix build_fake_color_output_matrix(const ContractMetadata& metadata,
                                                 const ComplianceGateResult& compliance_gate,
                                                 std::string input_signal,
                                                 std::string target_display_class);

}  // namespace kivo::cinema_engine

