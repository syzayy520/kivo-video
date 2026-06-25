#include "kivo/cinema_engine/signing_core/fake_signing_service.hpp"

namespace kivo::cinema_engine {

// Code signing policy builders
CodeSigningPolicy FakeSigningService::build_strict_signing_policy() {
    CodeSigningPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "strict-signing-001";
    policy.signing_required = true;
    policy.certificate_validation = true;
    policy.timestamp_signing = true;
    policy.allowed_signers = {"kivo-video", "kivo-official"};
    policy.revocation_check = true;
    return policy;
}

CodeSigningPolicy FakeSigningService::build_relaxed_signing_policy() {
    CodeSigningPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "relaxed-signing-001";
    policy.signing_required = true;
    policy.certificate_validation = false;
    policy.timestamp_signing = false;
    policy.allowed_signers = {"kivo-video"};
    policy.revocation_check = false;
    return policy;
}

CodeSigningPolicy FakeSigningService::build_no_signing_policy() {
    CodeSigningPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "no-signing-001";
    policy.signing_required = false;
    policy.certificate_validation = false;
    policy.timestamp_signing = false;
    policy.allowed_signers = {};
    policy.revocation_check = false;
    return policy;
}

}  // namespace kivo::cinema_engine