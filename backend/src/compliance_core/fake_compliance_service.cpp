#include "kivo/cinema_engine/compliance_core/fake_compliance_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

ComplianceGateResult build_fake_compliance_gate_result(
    const ContractMetadata& metadata,
    std::string claim_level,
    std::vector<std::string> allowed_claims,
    std::vector<std::string> forbidden_claim_reasons) {
  ComplianceGateResult result;
  result.metadata = metadata;
  result.compliance_gate_id = "compliance.fake.gate.0001";
  result.compliance_trace_id = "compliance.trace.fake.0001";
  result.claim_level = std::move(claim_level);
  result.allowed_claims = std::move(allowed_claims);
  result.forbidden_claim_reasons = std::move(forbidden_claim_reasons);
  result.certification_state = "certified";
  result.fail_closed = false;
  return result;
}

}  // namespace kivo::cinema_engine
