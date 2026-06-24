#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

ComplianceGateResult build_fake_compliance_gate_result(
    const ContractMetadata& metadata,
    std::string claim_level,
    std::vector<std::string> allowed_claims,
    std::vector<std::string> forbidden_claim_reasons);

}  // namespace kivo::cinema_engine
