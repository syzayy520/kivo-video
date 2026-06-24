#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/diagnostic_core/diagnostic_bundle_manifest.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

DiagnosticBundleManifest build_fake_diagnostic_bundle_manifest(
    const ContractMetadata& metadata,
    const ComplianceGateResult& compliance_gate,
    std::string redaction_profile,
    std::vector<std::string> included_refs);

}  // namespace kivo::cinema_engine
