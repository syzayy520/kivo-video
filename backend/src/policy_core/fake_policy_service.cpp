#include "kivo/cinema_engine/policy_core/fake_policy_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

PolicyDecision build_fake_policy_decision(
    const ContractMetadata& metadata,
    std::string decision,
    std::vector<std::string> reasons,
    std::string user_visible_summary) {
  PolicyDecision policy;
  policy.metadata = metadata;
  policy.policy_decision_id = "policy.fake.decision.0001";
  policy.decision = std::move(decision);
  policy.reasons = std::move(reasons);
  policy.user_visible_summary = std::move(user_visible_summary);
  return policy;
}

}  // namespace kivo::cinema_engine
