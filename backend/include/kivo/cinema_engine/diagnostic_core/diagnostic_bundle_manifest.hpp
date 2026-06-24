#pragma once

#include <string>
#include <vector>

#include "kivo/cinema_engine/foundation/contract_metadata.hpp"

namespace kivo::cinema_engine {

struct DiagnosticBundleManifest {
  ContractMetadata metadata;
  std::string diagnostic_bundle_id;
  std::string redaction_profile;
  std::string redaction_state;
  std::vector<std::string> included_refs;
  std::vector<std::string> excluded_secret_kinds;
  bool export_requires_consent{true};
};

}  // namespace kivo::cinema_engine

