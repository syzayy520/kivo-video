#include <cassert>
#include <string>

#include "kivo/cinema_engine/diagnostic_core/diagnostic_redaction_gate.hpp"

int diagnostic_redaction_gate_test() {
  const auto result = kivo::cinema_engine::redact_diagnostic_payload(
      "token=secret cookie=session signed_url=https://example.test/file?sig=abc credential=plain");

  assert(result.allowed);
  assert(result.redacted_payload.find("secret") == std::string::npos);
  assert(result.redacted_payload.find("session") == std::string::npos);
  assert(result.redacted_payload.find("https://example.test") == std::string::npos);
  assert(result.redacted_payload.find("plain") == std::string::npos);

  return 0;
}

