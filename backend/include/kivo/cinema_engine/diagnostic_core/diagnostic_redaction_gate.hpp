#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace kivo::cinema_engine {

struct DiagnosticRedactionResult {
  bool allowed{false};
  std::string redacted_payload;
  std::vector<std::string> redacted_secret_kinds;
};

DiagnosticRedactionResult redact_diagnostic_payload(std::string_view payload);

}  // namespace kivo::cinema_engine

