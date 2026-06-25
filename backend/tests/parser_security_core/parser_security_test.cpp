#include <cassert>
#include <iostream>

#include "kivo/cinema_engine/parser_security_core/fake_parser_security_service.hpp"

namespace {

using namespace kivo::cinema_engine;

void test_strict_parser_security_policy() {
    auto policy = FakeParserSecurityService::build_strict_parser_security_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "strict-parser-security-001");
    assert(policy.max_input_size_bytes == 524288000);
    assert(policy.max_depth == 16);
    assert(policy.reject_unknown_boxes == true);
    assert(policy.timeout_ms == 5000);
    assert(policy.require_header_signature == true);
}

void test_relaxed_parser_security_policy() {
    auto policy = FakeParserSecurityService::build_relaxed_parser_security_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "relaxed-parser-security-001");
    assert(policy.max_input_size_bytes == 2147483648);
    assert(policy.max_depth == 64);
    assert(policy.reject_unknown_boxes == false);
    assert(policy.timeout_ms == 30000);
    assert(policy.require_header_signature == false);
}

void test_timeout_parser_security_policy() {
    auto policy = FakeParserSecurityService::build_timeout_parser_security_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "timeout-parser-security-001");
    assert(policy.max_input_size_bytes == 1073741824);
    assert(policy.max_depth == 32);
    assert(policy.reject_unknown_boxes == true);
    assert(policy.timeout_ms == 1000);
    assert(policy.require_header_signature == true);
}

void test_reject_unknown_boxes_policy() {
    auto policy = FakeParserSecurityService::build_reject_unknown_boxes_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "reject-unknown-boxes-001");
    assert(policy.max_input_size_bytes == 1073741824);
    assert(policy.max_depth == 32);
    assert(policy.reject_unknown_boxes == true);
    assert(policy.timeout_ms == 10000);
    assert(policy.require_header_signature == false);
}

void test_standard_quarantine_policy() {
    auto policy = FakeParserSecurityService::build_standard_quarantine_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "standard-quarantine-001");
    assert(policy.quarantine_duration_seconds == 86400);
    assert(policy.auto_delete_quarantined == false);
    assert(policy.max_quarantine_size_mb == 1024);
    assert(policy.notify_on_quarantine == true);
    assert(policy.escalation_threshold == 3);
}

void test_aggressive_quarantine_policy() {
    auto policy = FakeParserSecurityService::build_aggressive_quarantine_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "aggressive-quarantine-001");
    assert(policy.quarantine_duration_seconds == 604800);
    assert(policy.auto_delete_quarantined == true);
    assert(policy.max_quarantine_size_mb == 2048);
    assert(policy.notify_on_quarantine == true);
    assert(policy.escalation_threshold == 1);
}

void test_minimal_quarantine_policy() {
    auto policy = FakeParserSecurityService::build_minimal_quarantine_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "minimal-quarantine-001");
    assert(policy.quarantine_duration_seconds == 3600);
    assert(policy.auto_delete_quarantined == true);
    assert(policy.max_quarantine_size_mb == 256);
    assert(policy.notify_on_quarantine == false);
    assert(policy.escalation_threshold == 5);
}

void test_escalation_quarantine_policy() {
    auto policy = FakeParserSecurityService::build_escalation_quarantine_policy();
    assert(policy.metadata.schema_version == "1.0.0");
    assert(policy.policy_id == "escalation-quarantine-001");
    assert(policy.quarantine_duration_seconds == 259200);
    assert(policy.auto_delete_quarantined == false);
    assert(policy.max_quarantine_size_mb == 4096);
    assert(policy.notify_on_quarantine == true);
    assert(policy.escalation_threshold == 2);
}

void test_basic_fuzz_shell() {
    auto shell = FakeParserSecurityService::build_basic_fuzz_shell();
    assert(shell.metadata.schema_version == "1.0.0");
    assert(shell.shell_id == "basic-fuzz-shell-001");
    assert(shell.max_iterations == 10000);
    assert(shell.crash_reproduction == true);
    assert(shell.coverage_guided == false);
    assert(shell.seed_corpus_path == "/corpus/basic");
    assert(shell.mutation_strategy == "bitflip");
}

void test_coverage_guided_fuzz_shell() {
    auto shell = FakeParserSecurityService::build_coverage_guided_fuzz_shell();
    assert(shell.metadata.schema_version == "1.0.0");
    assert(shell.shell_id == "coverage-guided-fuzz-shell-001");
    assert(shell.max_iterations == 100000);
    assert(shell.crash_reproduction == true);
    assert(shell.coverage_guided == true);
    assert(shell.seed_corpus_path == "/corpus/coverage");
    assert(shell.mutation_strategy == "insert");
}

void test_quick_fuzz_shell() {
    auto shell = FakeParserSecurityService::build_quick_fuzz_shell();
    assert(shell.metadata.schema_version == "1.0.0");
    assert(shell.shell_id == "quick-fuzz-shell-001");
    assert(shell.max_iterations == 1000);
    assert(shell.crash_reproduction == false);
    assert(shell.coverage_guided == false);
    assert(shell.seed_corpus_path == "/corpus/quick");
    assert(shell.mutation_strategy == "delete");
}

void test_reproduction_fuzz_shell() {
    auto shell = FakeParserSecurityService::build_reproduction_fuzz_shell();
    assert(shell.metadata.schema_version == "1.0.0");
    assert(shell.shell_id == "reproduction-fuzz-shell-001");
    assert(shell.max_iterations == 50000);
    assert(shell.crash_reproduction == true);
    assert(shell.coverage_guided == true);
    assert(shell.seed_corpus_path == "/corpus/repro");
    assert(shell.mutation_strategy == "splice");
}

}  // namespace

int main() {
    std::cout << "Running P2-051 Media Parser Security / Fuzz tests..." << std::endl;

    test_strict_parser_security_policy();
    std::cout << "PASS: test_strict_parser_security_policy" << std::endl;

    test_relaxed_parser_security_policy();
    std::cout << "PASS: test_relaxed_parser_security_policy" << std::endl;

    test_timeout_parser_security_policy();
    std::cout << "PASS: test_timeout_parser_security_policy" << std::endl;

    test_reject_unknown_boxes_policy();
    std::cout << "PASS: test_reject_unknown_boxes_policy" << std::endl;

    test_standard_quarantine_policy();
    std::cout << "PASS: test_standard_quarantine_policy" << std::endl;

    test_aggressive_quarantine_policy();
    std::cout << "PASS: test_aggressive_quarantine_policy" << std::endl;

    test_minimal_quarantine_policy();
    std::cout << "PASS: test_minimal_quarantine_policy" << std::endl;

    test_escalation_quarantine_policy();
    std::cout << "PASS: test_escalation_quarantine_policy" << std::endl;

    test_basic_fuzz_shell();
    std::cout << "PASS: test_basic_fuzz_shell" << std::endl;

    test_coverage_guided_fuzz_shell();
    std::cout << "PASS: test_coverage_guided_fuzz_shell" << std::endl;

    test_quick_fuzz_shell();
    std::cout << "PASS: test_quick_fuzz_shell" << std::endl;

    test_reproduction_fuzz_shell();
    std::cout << "PASS: test_reproduction_fuzz_shell" << std::endl;

    std::cout << "\nAll 12 P2-051 tests passed!" << std::endl;
    return 0;
}