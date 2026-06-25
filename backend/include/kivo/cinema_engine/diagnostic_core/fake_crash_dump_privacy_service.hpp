#pragma once

#include "kivo/cinema_engine/diagnostic_core/crash_dump_privacy_policy.hpp"
#include "kivo/cinema_engine/diagnostic_core/support_bundle_redaction_policy.hpp"
#include "kivo/cinema_engine/diagnostic_core/privacy_consent_record.hpp"

namespace kivo::cinema_engine {

// Fake crash dump privacy service for testing.
// Provides static builder functions for crash dump privacy policy, support bundle redaction, and consent.
class FakeCrashDumpPrivacyService {
 public:
    // Crash dump policy builders
    static CrashDumpPrivacyPolicy build_full_collection_policy();
    static CrashDumpPrivacyPolicy build_redacted_collection_policy();
    static CrashDumpPrivacyPolicy build_no_collection_policy();
    static CrashDumpPrivacyPolicy build_fail_closed_policy();
    
    // Support bundle redaction builders
    static SupportBundleRedactionPolicy build_standard_redaction();
    static SupportBundleRedactionPolicy build_maximum_redaction();
    static SupportBundleRedactionPolicy build_minimal_redaction();
    static SupportBundleRedactionPolicy build_no_redaction();
    
    // Consent record builders
    static PrivacyConsentRecord build_given_consent();
    static PrivacyConsentRecord build_not_given_consent();
    static PrivacyConsentRecord build_revocable_consent();
    static PrivacyConsentRecord build_irrevocable_consent();
};

}  // namespace kivo::cinema_engine