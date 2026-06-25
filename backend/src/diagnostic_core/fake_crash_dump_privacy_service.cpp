#include "kivo/cinema_engine/diagnostic_core/fake_crash_dump_privacy_service.hpp"

namespace kivo::cinema_engine {

// Crash dump policy builders
CrashDumpPrivacyPolicy FakeCrashDumpPrivacyService::build_full_collection_policy() {
    CrashDumpPrivacyPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "full-collection-001";
    policy.collect_crash_dumps = true;
    policy.redact_pii = false;
    policy.redact_credentials = false;
    policy.fail_closed_on_redaction_failure = false;
    policy.retention_days = 90;
    policy.require_user_consent = true;
    return policy;
}

CrashDumpPrivacyPolicy FakeCrashDumpPrivacyService::build_redacted_collection_policy() {
    CrashDumpPrivacyPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "redacted-collection-001";
    policy.collect_crash_dumps = true;
    policy.redact_pii = true;
    policy.redact_credentials = true;
    policy.fail_closed_on_redaction_failure = true;
    policy.retention_days = 30;
    policy.require_user_consent = true;
    return policy;
}

CrashDumpPrivacyPolicy FakeCrashDumpPrivacyService::build_no_collection_policy() {
    CrashDumpPrivacyPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "no-collection-001";
    policy.collect_crash_dumps = false;
    policy.redact_pii = true;
    policy.redact_credentials = true;
    policy.fail_closed_on_redaction_failure = true;
    policy.retention_days = 0;
    policy.require_user_consent = false;
    return policy;
}

CrashDumpPrivacyPolicy FakeCrashDumpPrivacyService::build_fail_closed_policy() {
    CrashDumpPrivacyPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "fail-closed-001";
    policy.collect_crash_dumps = true;
    policy.redact_pii = true;
    policy.redact_credentials = true;
    policy.fail_closed_on_redaction_failure = true;
    policy.retention_days = 7;
    policy.require_user_consent = true;
    return policy;
}

// Support bundle redaction builders
SupportBundleRedactionPolicy FakeCrashDumpPrivacyService::build_standard_redaction() {
    SupportBundleRedactionPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "standard-redaction-001";
    policy.redact_user_paths = true;
    policy.redact_network_info = true;
    policy.redact_memory_contents = true;
    policy.redact_clipboard_data = true;
    policy.redaction_level = "standard";
    policy.fail_closed_on_redaction_failure = true;
    return policy;
}

SupportBundleRedactionPolicy FakeCrashDumpPrivacyService::build_maximum_redaction() {
    SupportBundleRedactionPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "maximum-redaction-001";
    policy.redact_user_paths = true;
    policy.redact_network_info = true;
    policy.redact_memory_contents = true;
    policy.redact_clipboard_data = true;
    policy.redaction_level = "maximum";
    policy.fail_closed_on_redaction_failure = true;
    return policy;
}

SupportBundleRedactionPolicy FakeCrashDumpPrivacyService::build_minimal_redaction() {
    SupportBundleRedactionPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "minimal-redaction-001";
    policy.redact_user_paths = false;
    policy.redact_network_info = false;
    policy.redact_memory_contents = true;
    policy.redact_clipboard_data = false;
    policy.redaction_level = "minimal";
    policy.fail_closed_on_redaction_failure = false;
    return policy;
}

SupportBundleRedactionPolicy FakeCrashDumpPrivacyService::build_no_redaction() {
    SupportBundleRedactionPolicy policy;
    policy.metadata.schema_version = "1.0.0";
    policy.policy_id = "no-redaction-001";
    policy.redact_user_paths = false;
    policy.redact_network_info = false;
    policy.redact_memory_contents = false;
    policy.redact_clipboard_data = false;
    policy.redaction_level = "none";
    policy.fail_closed_on_redaction_failure = false;
    return policy;
}

// Consent record builders
PrivacyConsentRecord FakeCrashDumpPrivacyService::build_given_consent() {
    PrivacyConsentRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "consent-given-001";
    record.user_id = "user-001";
    record.consent_given = true;
    record.consent_timestamp = 1719312000000;  // 2024-06-25
    record.consent_scope = "both";
    record.consent_version = "1.0.0";
    record.can_revoke = true;
    return record;
}

PrivacyConsentRecord FakeCrashDumpPrivacyService::build_not_given_consent() {
    PrivacyConsentRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "consent-not-given-001";
    record.user_id = "user-002";
    record.consent_given = false;
    record.consent_timestamp = 0;
    record.consent_scope = "";
    record.consent_version = "1.0.0";
    record.can_revoke = true;
    return record;
}

PrivacyConsentRecord FakeCrashDumpPrivacyService::build_revocable_consent() {
    PrivacyConsentRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "consent-revocable-001";
    record.user_id = "user-003";
    record.consent_given = true;
    record.consent_timestamp = 1719312000000;
    record.consent_scope = "crash_dumps";
    record.consent_version = "1.0.0";
    record.can_revoke = true;
    return record;
}

PrivacyConsentRecord FakeCrashDumpPrivacyService::build_irrevocable_consent() {
    PrivacyConsentRecord record;
    record.metadata.schema_version = "1.0.0";
    record.record_id = "consent-irrevocable-001";
    record.user_id = "user-004";
    record.consent_given = true;
    record.consent_timestamp = 1719312000000;
    record.consent_scope = "support_bundles";
    record.consent_version = "1.0.0";
    record.can_revoke = false;
    return record;
}

}  // namespace kivo::cinema_engine