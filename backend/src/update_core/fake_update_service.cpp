#include "kivo/cinema_engine/update_core/fake_update_service.hpp"

namespace kivo::cinema_engine {

// Update policy builders
UpdatePolicy FakeUpdateService::build_strict_update_policy() {
    UpdatePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "strict-update-001";
    policy.auto_update_enabled = true;
    policy.signature_verification = true;
    policy.rollback_enabled = true;
    policy.max_update_size_mb = 50;
    policy.staged_rollout = false;
    return policy;
}

UpdatePolicy FakeUpdateService::build_staged_update_policy() {
    UpdatePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "staged-update-001";
    policy.auto_update_enabled = true;
    policy.signature_verification = true;
    policy.rollback_enabled = true;
    policy.max_update_size_mb = 200;
    policy.staged_rollout = true;
    return policy;
}

UpdatePolicy FakeUpdateService::build_manual_update_policy() {
    UpdatePolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "manual-update-001";
    policy.auto_update_enabled = false;
    policy.signature_verification = true;
    policy.rollback_enabled = false;
    policy.max_update_size_mb = 500;
    policy.staged_rollout = false;
    return policy;
}

}  // namespace kivo::cinema_engine