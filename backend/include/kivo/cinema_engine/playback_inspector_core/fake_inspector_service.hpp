#pragma once

#include <string>

#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/playback_inspector_core/playback_inspector_snapshot.hpp"

namespace kivo::cinema_engine {

PlaybackInspectorSnapshot build_fake_playback_inspector_snapshot(
    const ContractMetadata& metadata,
    const ComplianceGateResult& compliance_gate,
    std::string user_summary,
    std::string source_summary,
    std::string network_summary);

}  // namespace kivo::cinema_engine
