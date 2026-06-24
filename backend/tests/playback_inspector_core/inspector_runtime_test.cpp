#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/playback_inspector_core/fake_inspector_runtime_service.hpp"
#include "kivo/cinema_engine/diagnostic_core/diagnostic_redaction_gate.hpp"

using namespace kivo::cinema_engine;

static void test_default_inspector_runtime() {
    auto runtime = build_default_inspector_runtime("inspector-001");
    assert(runtime.metadata.schema_version == "1.1");
    assert(runtime.inspector_id == "inspector-001");
    assert(runtime.reads_decision_ledger == true);
    assert(runtime.reads_telemetry_timeline == true);
    assert(runtime.reads_capability_join == true);
    assert(runtime.supports_redaction == true);
    std::cout << "  PASS: default_inspector_runtime\n";
}

static void test_decision_ledger_only_inspector() {
    auto runtime = build_decision_ledger_only_inspector("inspector-002");
    assert(runtime.reads_decision_ledger == true);
    assert(runtime.reads_telemetry_timeline == false);
    assert(runtime.reads_capability_join == false);
    assert(runtime.supports_redaction == true);
    std::cout << "  PASS: decision_ledger_only_inspector\n";
}

static void test_telemetry_only_inspector() {
    auto runtime = build_telemetry_only_inspector("inspector-003");
    assert(runtime.reads_decision_ledger == false);
    assert(runtime.reads_telemetry_timeline == true);
    assert(runtime.reads_capability_join == false);
    assert(runtime.supports_redaction == true);
    std::cout << "  PASS: telemetry_only_inspector\n";
}

static void test_no_redaction_inspector() {
    auto runtime = build_no_redaction_inspector("inspector-004");
    assert(runtime.reads_decision_ledger == true);
    assert(runtime.reads_telemetry_timeline == true);
    assert(runtime.reads_capability_join == true);
    assert(runtime.supports_redaction == false);
    std::cout << "  PASS: no_redaction_inspector\n";
}

static void test_disabled_inspector() {
    auto runtime = build_disabled_inspector("inspector-005");
    assert(runtime.reads_decision_ledger == false);
    assert(runtime.reads_telemetry_timeline == false);
    assert(runtime.reads_capability_join == false);
    assert(runtime.supports_redaction == false);
    std::cout << "  PASS: disabled_inspector\n";
}

static void test_redaction_with_inspector_data() {
    // Simulate a payload that contains secrets
    std::string payload = "session_id=abc123 token=secret123 cookie=xyz789 signed_url=https://example.com/file credential=plain";
    auto result = redact_diagnostic_payload(payload);
    assert(result.allowed == true);
    // Ensure secrets are redacted
    assert(result.redacted_payload.find("secret123") == std::string::npos);
    assert(result.redacted_payload.find("xyz783") == std::string::npos);
    assert(result.redacted_payload.find("plain") == std::string::npos);
    // Ensure redacted markers are present
    assert(result.redacted_payload.find("token=<redacted>") != std::string::npos);
    assert(result.redacted_payload.find("cookie=<redacted>") != std::string::npos);
    assert(result.redacted_payload.find("signed_url=<redacted>") != std::string::npos);
    assert(result.redacted_payload.find("credential=<redacted>") != std::string::npos);
    std::cout << "  PASS: redaction_with_inspector_data\n";
}

int main() {
    std::cout << "inspector_runtime_test:\n";
    test_default_inspector_runtime();
    test_decision_ledger_only_inspector();
    test_telemetry_only_inspector();
    test_no_redaction_inspector();
    test_disabled_inspector();
    test_redaction_with_inspector_data();
    std::cout << "ALL 6 TESTS PASSED\n";
    return 0;
}