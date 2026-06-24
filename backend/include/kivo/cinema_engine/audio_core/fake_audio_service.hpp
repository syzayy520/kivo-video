#pragma once

#include <string>

#include "kivo/cinema_engine/audio_core/audio_output_decision.hpp"
#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

AudioOutputDecision build_fake_audio_output_decision(const ContractMetadata& metadata,
                                                     const ComplianceGateResult& compliance_gate,
                                                     std::string selected_audio_stream,
                                                     bool passthrough_requested);

}  // namespace kivo::cinema_engine

