#pragma once

#include <optional>
#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct ComplianceGateResult {
  ContractMetadata metadata;
  std::string compliance_gate_id;
  std::string compliance_trace_id;
  std::string claim_level;
  std::vector<std::string> allowed_claims;
  std::vector<std::string> forbidden_claim_reasons;
  std::optional<std::string> certification_state;
  bool fail_closed{false};
};

}  // namespace kivo::cinema_engine

