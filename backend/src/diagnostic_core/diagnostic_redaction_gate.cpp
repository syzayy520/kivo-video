#include "kivo/cinema_engine/diagnostic_core/diagnostic_redaction_gate.hpp"

#include <array>

namespace kivo::cinema_engine {

namespace {

struct SecretMarker {
  std::string_view marker;
  std::string_view kind;
};

constexpr std::array<SecretMarker, 4> kSecretMarkers = {{
    {"token=", "token"},
    {"cookie=", "cookie"},
    {"signed_url=", "signed_url"},
    {"credential=", "credential"},
}};

void redact_marker(std::string& payload,
                   std::string_view marker,
                   std::string_view replacement,
                   std::vector<std::string>& kinds,
                   std::string_view kind) {
  std::size_t cursor = 0;
  while ((cursor = payload.find(marker, cursor)) != std::string::npos) {
    const std::size_t value_start = cursor + marker.size();
    std::size_t value_end = payload.find_first_of(" \n\r\t&", value_start);
    if (value_end == std::string::npos) {
      value_end = payload.size();
    }
    payload.replace(value_start, value_end - value_start, replacement);
    kinds.emplace_back(kind);
    cursor = value_start + replacement.size();
  }
}

}  // namespace

DiagnosticRedactionResult redact_diagnostic_payload(std::string_view payload) {
  DiagnosticRedactionResult result;
  result.redacted_payload = std::string(payload);

  for (const auto& marker : kSecretMarkers) {
    redact_marker(result.redacted_payload, marker.marker, "<redacted>", result.redacted_secret_kinds, marker.kind);
  }

  result.allowed = result.redacted_secret_kinds.empty() ||
                   (result.redacted_payload.find("token=secret") == std::string::npos &&
                    result.redacted_payload.find("cookie=session") == std::string::npos &&
                    result.redacted_payload.find("signed_url=https://") == std::string::npos &&
                    result.redacted_payload.find("credential=plain") == std::string::npos);
  return result;
}

}  // namespace kivo::cinema_engine

