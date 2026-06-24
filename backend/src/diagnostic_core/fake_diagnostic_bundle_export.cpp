#include "kivo/cinema_engine/diagnostic_core/fake_diagnostic_bundle_export.hpp"

#include <utility>

namespace kivo::cinema_engine {

DiagnosticBundleManifest build_fake_diagnostic_bundle_manifest(
    const ContractMetadata& metadata,
    const ComplianceGateResult& compliance_gate,
    std::string redaction_profile,
    std::vector<std::string> included_refs) {
  DiagnosticBundleManifest manifest;
  manifest.metadata = metadata;
  manifest.diagnostic_bundle_id = "diagnostic.fake.bundle.0001";
  manifest.redaction_profile = std::move(redaction_profile);
  manifest.redaction_state = compliance_gate.fail_closed ? "redacted" : "unredacted";
  manifest.included_refs = std::move(included_refs);
  manifest.excluded_secret_kinds = {"token", "cookie", "signed_url", "credential"};
  manifest.export_requires_consent = !compliance_gate.fail_closed;
  return manifest;
}

}  // namespace kivo::cinema_engine
