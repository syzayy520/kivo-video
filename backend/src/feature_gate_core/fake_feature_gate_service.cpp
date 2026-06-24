#include "kivo/cinema_engine/feature_gate_core/fake_feature_gate_service.hpp"

#include <utility>

namespace kivo::cinema_engine {

FeatureGateSnapshot build_fake_feature_gate_snapshot(
    const ContractMetadata& metadata,
    std::vector<std::string> enabled_features,
    std::vector<std::string> disabled_features,
    std::vector<std::string> experimental_features) {
  FeatureGateSnapshot snapshot;
  snapshot.metadata = metadata;
  snapshot.feature_gate_snapshot_id = "feature_gate.fake.snapshot.0001";
  snapshot.enabled_features = std::move(enabled_features);
  snapshot.disabled_features = std::move(disabled_features);
  snapshot.experimental_features = std::move(experimental_features);
  snapshot.compliance_limited_features = {"hdr10_plus", "dolby_vision"};
  snapshot.reason = "fake.service.default";
  snapshot.created_at = "2026-06-25T00:00:00Z";
  return snapshot;
}

}  // namespace kivo::cinema_engine
