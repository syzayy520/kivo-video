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

static void test_snapshot_read_request() {
    auto r = build_snapshot_read_request("inspector-001", "session-abc");
    assert(r.request_id == "req-snapshot-session-abc");
    assert(r.source_type == "snapshot");
    assert(r.session_id == "session-abc");
    assert(r.include_redacted == false);
    std::cout << "  PASS: snapshot_read_request\n";
}

static void test_ledger_read_request() {
    auto r = build_ledger_read_request("inspector-001", "session-def");
    assert(r.request_id == "req-ledger-session-def");
    assert(r.source_type == "ledger");
    assert(r.session_id == "session-def");
    std::cout << "  PASS: ledger_read_request\n";
}

static void test_successful_read_result() {
    auto res = build_successful_read_result("req-snapshot-session-abc", "snapshot", 42);
    assert(res.record_count == 42);
    assert(res.was_redacted == false);
    assert(res.source_type == "snapshot");
    assert(!res.data_payload.empty());
    std::cout << "  PASS: successful_read_result\n";
}

static void test_redacted_read_result() {
    auto res = build_redacted_read_result("req-ledger-session-def", "ledger");
    assert(res.was_redacted == true);
    assert(res.record_count == 5);
    assert(res.data_payload.find("redacted") != std::string::npos);
    std::cout << "  PASS: redacted_read_result\n";
}

int main() {
    std::cout << "inspector_runtime_test:\n";
    // Existing tests
    test_default_inspector_runtime();
    test_decision_ledger_only_inspector();
    test_telemetry_only_inspector();
    test_no_redaction_inspector();
    test_disabled_inspector();
    test_redaction_with_inspector_data();
    // P2-042 new tests
    test_snapshot_read_request();
    test_ledger_read_request();
    test_successful_read_result();
    test_redacted_read_result();
    std::cout << "ALL 10 TESTS PASSED\n";
    return 0;
}