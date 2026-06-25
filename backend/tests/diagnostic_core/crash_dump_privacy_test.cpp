#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/diagnostic_core/fake_crash_dump_privacy_service.hpp"

using namespace kivo::cinema_engine;

void test_crash_dump_privacy_policy_construction() {
    auto policy = FakeCrashDumpPrivacyService::build_full_collection_policy();
    assert(!policy.policy_id.empty());
    assert(policy.retention_days >= 0);
    std::cout << "PASS: test_crash_dump_privacy_policy_construction" << std::endl;
}

void test_support_bundle_redaction_policy_construction() {
    auto policy = FakeCrashDumpPrivacyService::build_standard_redaction();
    assert(!policy.policy_id.empty());
    assert(!policy.redaction_level.empty());
    std::cout << "PASS: test_support_bundle_redaction_policy_construction" << std::endl;
}

void test_privacy_consent_record_construction() {
    auto record = FakeCrashDumpPrivacyService::build_given_consent();
    assert(!record.record_id.empty());
    assert(!record.user_id.empty());
    assert(!record.consent_scope.empty());
    std::cout << "PASS: test_privacy_consent_record_construction" << std::endl;
}

void test_fail_closed_policy_on_redaction_failure() {
    auto policy = FakeCrashDumpPrivacyService::build_fail_closed_policy();
    assert(policy.fail_closed_on_redaction_failure == true);
    assert(policy.redact_pii == true);
    assert(policy.redact_credentials == true);
    std::cout << "PASS: test_fail_closed_policy_on_redaction_failure" << std::endl;
}

void test_fail_open_policy_on_redaction_failure() {
    auto policy = FakeCrashDumpPrivacyService::build_full_collection_policy();
    assert(policy.fail_closed_on_redaction_failure == false);
    assert(policy.redact_pii == false);
    assert(policy.redact_credentials == false);
    std::cout << "PASS: test_fail_open_policy_on_redaction_failure" << std::endl;
}

void test_maximum_redaction_level_policy() {
    auto policy = FakeCrashDumpPrivacyService::build_maximum_redaction();
    assert(policy.redaction_level == "maximum");
    assert(policy.redact_user_paths == true);
    assert(policy.redact_network_info == true);
    assert(policy.redact_memory_contents == true);
    assert(policy.redact_clipboard_data == true);
    std::cout << "PASS: test_maximum_redaction_level_policy" << std::endl;
}

void test_minimal_redaction_level_policy() {
    auto policy = FakeCrashDumpPrivacyService::build_minimal_redaction();
    assert(policy.redaction_level == "minimal");
    assert(policy.redact_user_paths == false);
    assert(policy.redact_network_info == false);
    assert(policy.redact_memory_contents == true);
    assert(policy.redact_clipboard_data == false);
    std::cout << "PASS: test_minimal_redaction_level_policy" << std::endl;
}

void test_user_consent_required_policy() {
    auto policy = FakeCrashDumpPrivacyService::build_redacted_collection_policy();
    assert(policy.require_user_consent == true);
    assert(policy.collect_crash_dumps == true);
    std::cout << "PASS: test_user_consent_required_policy" << std::endl;
}

void test_user_consent_not_required_policy() {
    auto policy = FakeCrashDumpPrivacyService::build_no_collection_policy();
    assert(policy.require_user_consent == false);
    assert(policy.collect_crash_dumps == false);
    std::cout << "PASS: test_user_consent_not_required_policy" << std::endl;
}

void test_consent_revocation_policy() {
    auto record = FakeCrashDumpPrivacyService::build_revocable_consent();
    assert(record.can_revoke == true);
    assert(record.consent_given == true);
    assert(record.consent_scope == "crash_dumps");
    std::cout << "PASS: test_consent_revocation_policy" << std::endl;
}

int main() {
    test_crash_dump_privacy_policy_construction();
    test_support_bundle_redaction_policy_construction();
    test_privacy_consent_record_construction();
    test_fail_closed_policy_on_redaction_failure();
    test_fail_open_policy_on_redaction_failure();
    test_maximum_redaction_level_policy();
    test_minimal_redaction_level_policy();
    test_user_consent_required_policy();
    test_user_consent_not_required_policy();
    test_consent_revocation_policy();
    
    std::cout << "All crash dump privacy tests passed." << std::endl;
    return 0;
}