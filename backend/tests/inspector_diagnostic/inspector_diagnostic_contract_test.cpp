#include <cassert>

#include "kivo/cinema_engine/compliance_core/compliance_gate_result.hpp"
#include "kivo/cinema_engine/diagnostic_core/fake_diagnostic_bundle_export.hpp"
#include "kivo/cinema_engine/foundation/contract_metadata.hpp"
#include "kivo/cinema_engine/playback_inspector_core/fake_inspector_service.hpp"

int main() {
  const auto metadata = kivo::cinema_engine::make_contract_metadata("trace.fake.inspector-diagnostic", "session.fake.0001");

  kivo::cinema_engine::ComplianceGateResult compliance_gate;
  compliance_gate.metadata = metadata;
  compliance_gate.compliance_gate_id = "compliance.fake.inspector";
  compliance_gate.compliance_trace_id = "compliance.trace.fake.inspector";
  compliance_gate.claim_level = "generic_playback";

  // Test PlaybackInspectorSnapshot
  const auto inspector_snapshot = kivo::cinema_engine::build_fake_playback_inspector_snapshot(
      metadata,
      compliance_gate,
      "user.fake.summary",
      "source.fake.summary",
      "network.fake.summary");
  assert(inspector_snapshot.metadata.schema_version == "1.1");
  assert(inspector_snapshot.inspector_snapshot_id == "inspector.fake.service.0001");
  assert(inspector_snapshot.user_summary == "user.fake.summary");
  assert(inspector_snapshot.source_summary == "source.fake.summary");
  assert(inspector_snapshot.network_summary == "network.fake.summary");
  assert(inspector_snapshot.redaction_state == "unredacted");
  assert(inspector_snapshot.actions_available.size() == 5);

  // Test DiagnosticBundleManifest
  const auto diagnostic_manifest = kivo::cinema_engine::build_fake_diagnostic_bundle_manifest(
      metadata,
      compliance_gate,
      "profile.full",
      {"ref.video", "ref.audio", "ref.subtitle"});
  assert(diagnostic_manifest.metadata.schema_version == "1.1");
  assert(diagnostic_manifest.diagnostic_bundle_id == "diagnostic.fake.bundle.0001");
  assert(diagnostic_manifest.redaction_profile == "profile.full");
  assert(diagnostic_manifest.redaction_state == "unredacted");
  assert(diagnostic_manifest.included_refs.size() == 3);
  assert(diagnostic_manifest.excluded_secret_kinds.size() == 4);
  assert(diagnostic_manifest.export_requires_consent == true);

  // Test with fail_closed compliance gate
  compliance_gate.fail_closed = true;
  const auto redacted_inspector = kivo::cinema_engine::build_fake_playback_inspector_snapshot(
      metadata,
      compliance_gate,
      "user.fake.redacted.summary",
      "source.fake.redacted.summary",
      "network.fake.redacted.summary");
  assert(redacted_inspector.redaction_state == "redacted");

  const auto redacted_diagnostic = kivo::cinema_engine::build_fake_diagnostic_bundle_manifest(
      metadata,
      compliance_gate,
      "profile.redacted",
      {"ref.redacted"});
  assert(redacted_diagnostic.redaction_state == "redacted");
  assert(redacted_diagnostic.export_requires_consent == false);

  return 0;
}
